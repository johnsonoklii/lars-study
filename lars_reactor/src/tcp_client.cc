#include "tcp_client.h"
#include "buf_pool.h"
#include "message.h"
#include "handler.h"

#include <string>

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>

tcp_client::tcp_client(event_loop* loop, const char* ip, int port)
: m_loop(loop)
, m_sock(csocket::create_socket(AF_INET)) {
    bzero(&m_svr_addr, sizeof(m_svr_addr));
    m_svr_addr.sin_family = AF_INET;
    m_svr_addr.sin_port = htons(port);
    m_svr_addr.sin_addr.s_addr = inet_addr(ip);
}

void tcp_client::do_connect() {

    int ret = ::connect(m_sock.fd(), (sockaddr*)&m_svr_addr, sizeof(m_svr_addr));
    if (ret > 0) {
        // 连接成功
        printf("ret > 0 connect success\n");
        m_connected = true;

        // 1. 注册可读事件
        m_loop->add_io_event(m_sock.fd(), EPOLLIN, std::bind(&tcp_client::handle_read, this));

        if (m_on_build_callback) {
            m_on_build_callback(shared_from_this());
        }

    } else if (ret < 0) {
        if (errno == EINPROGRESS) {
            // 连接中
            // 1. 注册可写事件
            m_loop->add_io_event(m_sock.fd(), EPOLLOUT, std::bind(&tcp_client::handle_connect, this));
        } else {
            // 连接失败
            fprintf(stderr, "connect error: %s\n", strerror(errno));
            exit(1);
        }
    }

    return;
}

void tcp_client::handle_connect() {
    m_loop->del_io_event(m_sock.fd());
    
    int ret = 0;
    socklen_t ret_len = sizeof(ret);
    getsockopt(m_sock.fd(), SOL_SOCKET, SO_ERROR, &ret, &ret_len);
    if (ret == 0) {
        m_connected = true;
        printf("connect succ!\n");

        // 1. 注册读事件
        m_loop->add_io_event(m_sock.fd(), EPOLLIN, std::bind(&tcp_client::handle_read, this));

        if (m_on_build_callback) {
            m_on_build_callback(shared_from_this());
        }

    } else {
        fprintf(stderr, "connect error: %s\n", strerror(ret));
        exit(1);
    }
}

void tcp_client::handle_read() {
    assert(m_connected);
    int need_read = 0;
    if (ioctl(m_sock.fd(), FIONREAD, &need_read) == -1) {
        fprintf(stderr, "ioctl FIONREAD error");
        return;
    }

    int has_read = m_ibuf.read_data(m_sock.fd());
    if (has_read < 0) {
        fprintf(stderr, "read error: %s\n", strerror(errno));
        clean_conn();
        return;
    } else if (has_read == 0) {
        // 对端关闭
        printf("peer close!\n");
        clean_conn();
        return;
    }

     // 1. 解析业务数据
    while (m_ibuf.length() >= MESSAGE_HEADER_SIZE) {
        const char* data = m_ibuf.data();
        // 1. 解析header
        struct message msg;
        bzero(&msg, MESSAGE_HEADER_SIZE);
        memcpy(&msg, data, MESSAGE_HEADER_SIZE);

        int msg_len = msg.len;
        if (msg_len > MESSAGE_MAX_SIZE || msg_len < 0) {
            fprintf(stderr, "tcp_conn::handle_read message error: %s\n", strerror(errno));
            clean_conn();
            break;
        }
     
        if (m_ibuf.length() < msg_len) {
            break;
        }

        // 解析body
        int body_len = msg_len - MESSAGE_HEADER_SIZE;
        char body[body_len];
        bzero(body, body_len);
        memcpy(body, data + MESSAGE_HEADER_SIZE, body_len);

        int msgid = msg.id;
        // 根据msgid分配处理函数
        handler_t handler = handler_map::get_instance().get_handler(msgid);
        if (handler) {
            handler(shared_from_this(), msgid, body, body_len);
        }

        m_ibuf.pop(msg_len);
    }
    m_ibuf.adjust();
}

void tcp_client::handle_write() {
    while(m_obuf.length() > 0) {
        int has_write = m_obuf.write2fd(m_sock.fd());
        if (has_write < 0) {
            fprintf(stderr, "write error\n");
            clean_conn();
            break;;
        } else if (has_write == 0) {
            // EAGAIN
            // TODO: 如果是ET模式，需要重新注册
            break;
        }
    }
    
    if (m_obuf.length() == 0) {
        m_loop->del_io_event(m_sock.fd(), EPOLLOUT);
    }
}

int tcp_client::send(int msgid, const char* data, int len) {
    if (m_connected == false) {
        fprintf(stderr, "no connected , send message stop!\n");
        return -1;
    }

    bool activate_epollout = false;
    if (m_obuf.length() == 0) {
        activate_epollout = true;
    }

    // 1. 封装数据包
    struct message res_msg;
    bzero(&res_msg, MESSAGE_HEADER_SIZE);
    res_msg.id = msgid;
    res_msg.len = MESSAGE_HEADER_SIZE + len;

    int ret = 0;
    ret = m_obuf.write_data((char*)&res_msg, MESSAGE_HEADER_SIZE);
    if (ret < 0) {
        fprintf(stderr, "send head error\n");
        return -1;
    }

    ret = m_obuf.write_data(data, len);
    if (ret < 0) {
        fprintf(stderr, "send body error\n");
        m_obuf.pop(MESSAGE_HEADER_SIZE); // 将header弹出
        return -1;
    }

    // 2. 注册可写事件
    if (activate_epollout) {
        m_loop->add_io_event(m_sock.fd(), EPOLLOUT, std::bind(&tcp_client::handle_write, this));
    }

    return 0;
}

void tcp_client::clean_conn() {
    if (m_sock.fd() != -1) {
        printf("clean conn, del socket!\n");
        m_loop->del_io_event(m_sock.fd()); // 不删除事件，对端关闭了就会一直触发可读事件
    }

    m_ibuf.clear();
    m_obuf.clear();

    m_connected = false;

    if (m_on_close_callback) {
        m_on_close_callback(shared_from_this());
    }

    //重新连接
    do_connect();
}
#include "tcp_conn.h"
#include "message.h"
#include "tcp_server.h"
#include "handler.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <string>

tcp_conn::tcp_conn(event_loop* loop, int connfd)
: m_loop(loop), m_sock(connfd) {
    tcp_server::s_conn_nums++;

    m_sock.set_nodelay(true);
    // m_loop->add_io_event(m_sock.fd(), EPOLLIN, std::bind(&tcp_conn::handle_read, shared_from_this()));
    m_state = CONN_STATE::kConnected;
}

tcp_conn::~tcp_conn() {
    printf("tcp_conn::~tcp_conn()\n");
    close();
    tcp_server::s_conn_nums--;
}

void tcp_conn::listen_read() {
    m_loop->add_io_event(m_sock.fd(), EPOLLIN, std::bind(&tcp_conn::handle_read, shared_from_this()));

    if (m_on_build_callback) {
        m_on_build_callback(shared_from_this());
    }

    t_event_loop_conns[m_sock.fd()] = shared_from_this();
}

void tcp_conn::close() {
    if (m_state != CONN_STATE::kConnected) {
        return;
    }
    m_state = CONN_STATE::kDisconnected;
    m_loop->del_io_event(m_sock.fd());
    m_ibuf.clear(); 
    m_obuf.clear();

    t_event_loop_conns.erase(m_sock.fd());

    m_close_callback();
}

int tcp_conn::send(int msgid, const char* data, int len) {
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
        m_loop->add_io_event(m_sock.fd(), EPOLLOUT, std::bind(&tcp_conn::handle_write, shared_from_this()));
    }

    return 0;
}

void tcp_conn::handle_read() {
    int has_read = m_ibuf.read_data(m_sock.fd());
    if (has_read < 0) {
        fprintf(stderr, "tcp_conn::handle_read read error: %s\n", strerror(errno));
        handle_close();
        return;
    } else if (has_read == 0) {
        handle_close();
        return;
    }
    // 1. 解析业务数据
    const char* data = m_ibuf.data();
    while (m_ibuf.length() >= MESSAGE_HEADER_SIZE) {
        // 1. 解析header
        struct message msg;
        bzero(&msg, MESSAGE_HEADER_SIZE);
        memcpy(&msg, data, MESSAGE_HEADER_SIZE);

        int msg_len = msg.len;

        if (msg_len > MESSAGE_MAX_SIZE || msg_len < 0) {
            fprintf(stderr, "tcp_conn::handle_read message error: %s\n", strerror(errno));
            handle_close();
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

void tcp_conn::handle_write() {
    while(m_obuf.length() > 0) {
        // printf("m_obuf.length():  %d\n", m_obuf.length());
        int has_write = m_obuf.write2fd(m_sock.fd());
        if (has_write < 0) {
            fprintf(stderr, "write error\n");
            handle_close();
            break;;
        } else if (has_write == 0) {
            break;
        }
    }
    
    if (m_obuf.length() == 0) {
        m_loop->del_io_event(m_sock.fd(), EPOLLOUT);
    }
}

void tcp_conn::handle_error() {

}

void tcp_conn::handle_close() {
    fprintf(stderr, "tcp_conn::handle_close\n");
    close();
}
#include "udp_server.h"
#include "message.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

udp_server::udp_server(event_loop* loop, const char* ip, uint16_t port)
: m_loop(loop)
, m_socket(csocket::create_socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP)) {
    //1 忽略一些信号
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        perror("signal ignore SIGHUB");
        exit(1);
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal ignore SIGPIPE");
        exit(1);
    }

    bzero(m_read_buf, sizeof(m_read_buf));
    bzero(m_write_buf, sizeof(m_write_buf));

    bzero(&m_client_addr, sizeof(m_client_addr));
    m_client_addrlen = sizeof(m_client_addr);

    // 地址
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    // bind
    int ret = bind(m_socket.fd(), (const struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        exit(1);
    }

    // 添加lfd可读事件
    m_loop->add_io_event(m_socket.fd(), EPOLLIN, std::bind(&udp_server::do_read, this));
}

udp_server::~udp_server() {
    m_loop->del_io_event(m_socket.fd());
}

void udp_server::do_read() {
    
    while (true) {
        int pkg_len = recvfrom(m_socket.fd(), m_read_buf, sizeof(m_read_buf), 0, (struct sockaddr *)&m_client_addr, &m_client_addrlen);
        if (pkg_len == -1) {
            if (errno == EINTR) {
                continue;
            }
            else if (errno == EAGAIN) {
                break;
            }
            else {
                perror("recvfrom...\n");
                break;
            }
        }

        message head; 
        memcpy(&head, m_read_buf, MESSAGE_HEADER_SIZE);
        if (head.len > MESSAGE_MAX_SIZE || head.len <= 0 || head.len != pkg_len) {
            fprintf(stderr, "do_read, data error, msgid = %d, msglen = %d, pkg_len = %d\n", head.id, head.len, pkg_len);
            continue;
        }

        int msgid = head.id;
        // 根据msgid分配处理函数
        handler_t handler = handler_map::get_instance().get_handler(msgid);
        if (handler) {
            handler(shared_from_this(), msgid, m_read_buf + MESSAGE_HEADER_SIZE, pkg_len - MESSAGE_HEADER_SIZE);
        }
    }
}

int udp_server::send(int msgid, const char* data, int len) {

    if (len > MESSAGE_MAX_SIZE - MESSAGE_HEADER_SIZE) {
        fprintf(stderr, "too large message to send\n");
        return -1;
    }

    // 1. header
    message head;
    head.id = msgid;
    head.len = MESSAGE_HEADER_SIZE + len;

    // 2. msg
    memcpy(m_write_buf, &head, MESSAGE_HEADER_SIZE);
    memcpy(m_write_buf + MESSAGE_HEADER_SIZE, data, len);

    // 3. 发送
    int ret = sendto(m_socket.fd(), m_write_buf, head.len, 0, (struct sockaddr*)&m_client_addr, m_client_addrlen);
    if (ret < 0) {
        perror("sendto...");
        return -1;
    }

    return ret;
}


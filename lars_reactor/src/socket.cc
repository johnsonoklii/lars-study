#include "socket.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

int csocket::create_socket(sa_family_t family, int type, int protocol) {
    int sockfd = ::socket(family, type, protocol);
    if (sockfd < 0) {
        fprintf(stderr, "create_socket error: %s\n", strerror(errno));
        exit(1);
    }
    return sockfd;
}

csocket::csocket(int fd)
: m_fd(fd) {

}

csocket::~csocket() {
    // printf("csocket::~csocket()\n");
    if (m_fd > 0) {
        ::close(m_fd);
    }
}

void csocket::bind(sockaddr_in server_addr) {
    int ret = ::bind(m_fd, (struct sockaddr*)&server_addr, sizeof(sockaddr));
    if (ret < 0) {
        fprintf(stderr, "csocket::bind() error: %s\n", strerror(errno));
        exit(1);
    }
}

void csocket::listen() {
    if (::listen(m_fd, 5) < 0) {
        fprintf(stderr, "csocket::listen()\n");
        exit(1);
    } 
}

int csocket::accept(sockaddr_in* peer_addr) {
    bzero(peer_addr, sizeof(sockaddr_in));
    socklen_t addrlen = sizeof(sockaddr);
    int connfd = ::accept4(m_fd, (sockaddr*)peer_addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        int save_errno = errno;
        fprintf(stderr, "csocket::accept4()\n");
        switch (save_errno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = save_errno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                fprintf(stderr, "unexpected error of ::accept4 %d\n", save_errno);
                break;
            default:
                fprintf(stderr, "unknown error of ::accept4 %d\n", save_errno);
                break;
        }
    }

    return connfd;
}

void csocket::set_nonblocking() {
    int flag = fcntl(m_fd, F_GETFL, 0);
    if (flag < 0) {
        fprintf(stderr, "fcntl get flags error\n");
        return;
    }
    int ret = fcntl(m_fd, F_SETFL, O_NONBLOCK|flag);
    if (ret < 0) {
        fprintf(stderr, "fcntl set nonblocking error\n");
        return;
    }
}

void csocket::set_reuse_addr(bool on) {
    int op = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
}

void csocket::set_reuse_port(bool on) {
    int op = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &op, sizeof(op));
}

void csocket::set_nodelay(bool on) {
    int op = on ? 1 : 0;
    ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));
}

 void csocket::set_keepalive(bool on) {
    int op = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &op, sizeof(op));
 }
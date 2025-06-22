#pragma once

#include <sys/socket.h>
#include <netinet/in.h>

class csocket {
public:
    explicit csocket(int fd);
    csocket() = default;
    ~csocket();

    void bind(sockaddr_in server_addr);
    void listen();
    int accept(sockaddr_in* peer_addr);

    void set_nonblocking();
    void set_reuse_addr(bool on);
    void set_reuse_port(bool on);
    void set_nodelay(bool on);
    void set_keepalive(bool on);

    int fd() const {  return m_fd; }
    void set_fd(int fd) { m_fd = fd; }

    static int create_socket(sa_family_t family, int type, int protocol);
private:
    int m_fd;
};
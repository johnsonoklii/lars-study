#pragma once

#include "event_loop.h"
#include "socket.h"
#include "net_connection.h"
#include "handler.h"

#include <memory>

class udp_server: public net_connection, public std::enable_shared_from_this<udp_server> {
public:
    using sptr = std::shared_ptr<udp_server>;
    udp_server(event_loop* loop, const char* ip, uint16_t port);
    ~udp_server();

    void add_router(int msgid, handler_t handler) { handler_map::get_instance().register_handler(msgid, handler); }

    int fd() { return m_socket.fd(); }

    int send(int msgid, const char* data, int len);

private:
    void do_read();

private:
    event_loop* m_loop;
    csocket m_socket;

    char m_read_buf[1024];
    char m_write_buf[1024];

    sockaddr_in m_client_addr;
    socklen_t m_client_addrlen;
};
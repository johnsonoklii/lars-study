#pragma once

#include "event_loop.h"
#include "socket.h"
#include "io_buf.h"
#include "reactor_buf.h"
#include "net_connection.h"
#include "handler.h"

#include <memory>
#include <stdio.h>
#include <netinet/in.h>
#include <functional>

// TODO: 回调函数放在一个统一的头文件
using on_build_callback_t = std::function<void(const net_connection::sptr&)>;
using on_close_callback_t = std::function<void(const net_connection::sptr&)>;

class tcp_client: public net_connection, public std::enable_shared_from_this<tcp_client> {
public:
    using sptr = std::shared_ptr<tcp_client>;
    tcp_client(event_loop* loop, const char* ip, int port);
    ~tcp_client() {
        printf("tcp_client::~tcp_client()\n");
    }
    
    void do_connect();

    int fd() override { return m_sock.fd(); }

    int send(int msgid, const char* data, int len) override;
    void clean_conn();

    void set_on_build_callback(on_build_callback_t cb) { m_on_build_callback =  cb; }
    void set_on_close_callback(on_close_callback_t cb) { m_on_close_callback = cb; }

    void add_router(int msgid, handler_t handler) { handler_map::get_instance().register_handler(msgid, handler); }

private:
    void handle_connect();
    void handle_read();
    void handle_write();

private:
    event_loop* m_loop{nullptr};
    csocket  m_sock;

    struct sockaddr_in m_svr_addr;
    
    input_buf m_ibuf;
    output_buf m_obuf;

    bool m_connected{false};

    on_build_callback_t m_on_build_callback{nullptr};
    on_close_callback_t m_on_close_callback{nullptr};
};
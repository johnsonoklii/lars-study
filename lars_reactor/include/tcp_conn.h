#pragma once

#include "socket.h"
#include "reactor_buf.h"
#include "event_loop.h"
#include "net_connection.h"

#include <memory>

using close_callback_t = std::function<void()>;
using on_build_callback_t = std::function<void(const net_connection::sptr&)>;

class tcp_conn: public net_connection, public std::enable_shared_from_this<tcp_conn> {
public:
    using sptr = std::shared_ptr<tcp_conn>;

    enum class CONN_STATE {
        kDisconnected, kConnecting, kConnected, kDisconnecting
    };

    tcp_conn(event_loop* loop, int connfd);
    ~tcp_conn();

    int fd() override { return m_sock.fd(); }

    int send(int msgid, const char* data, int len) override;
    void listen_read();
    void close();

    void set_close_callback(close_callback_t cb) { m_close_callback = cb; }
    void set_on_build_callback(on_build_callback_t cb) { m_on_build_callback =  cb; }

private:
    void handle_read();
    void handle_write();
    void handle_error(); 
    void handle_close();
    
private:
    event_loop* m_loop{nullptr};
    csocket m_sock;

    CONN_STATE m_state{CONN_STATE::kDisconnected};
 
    input_buf  m_ibuf;
    output_buf m_obuf;

    on_build_callback_t m_on_build_callback{nullptr};
    close_callback_t m_close_callback;
};
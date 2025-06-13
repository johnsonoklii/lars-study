#pragma once

#include "tcp_conn.h"
#include "event_loop.h"
#include "socket.h"
#include "event_loop_thread_pool.h"
#include "handler.h"

#include <unordered_map>
#include <atomic>

#include <netinet/in.h>

using on_close_callback_t = std::function<void(const net_connection::sptr&)>;

class tcp_server {
public:
    tcp_server(event_loop* loop, const char* ip, uint16_t port);
    ~tcp_server();

    // static tcp_conn::sptr get_conn(int fd);

    void set_thraed_num(int num) { m_thread_pool->set_thread_num(num); }

    void start();

    void set_on_build_callback(on_build_callback_t cb) { m_on_build_callback =  cb; }
    void set_on_close_callback(on_close_callback_t cb) { m_on_close_callback = cb; }
    void add_router(int msgid, handler_t handler) { handler_map::get_instance().register_handler(msgid, handler); }

    void send_task(task_callback_t cb);
    
    static std::atomic<int> s_conn_nums;
    
private:
    void accept_callback();
    void delete_conn(int fd);
    void delete_conn_inloop(int fd);

private:
    csocket m_listen_sock;
    event_loop* m_loop{nullptr};
    std::unordered_map<int, tcp_conn::sptr> m_conns; // TODO: 这是必须的吗
    event_loop_thread_pool::uptr m_thread_pool;

    int m_max_conn_nums{1024};

    on_build_callback_t m_on_build_callback{nullptr};
    on_close_callback_t m_on_close_callback{nullptr};
};
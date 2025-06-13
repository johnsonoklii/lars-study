#pragma once 

#include "event_base.h"
#include "net_connection.h"

#include <sys/epoll.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

using io_event_map = std::unordered_map<int, io_event>; // fd:io_event

constexpr int MAXEVENTS = 10;

class task_msg;
class task_queue;

using listen_fd_set = std::unordered_set<int>;

extern thread_local std::unordered_map<int, net_connection::sptr> t_event_loop_conns;

class event_loop {
public:
    event_loop();
    ~event_loop();

    void loop();

    listen_fd_set get_listen_fds() { return m_listen_fds; }

    void add_io_event(int fd, int event, io_callback_t proc);
    void del_io_event(int fd); 
    void del_io_event(int fd, int event);

    void add_task(const task_msg& task);
    
private:
    int m_efd{-1};
    io_event_map m_io_evs_map;
    listen_fd_set m_listen_fds;

    struct epoll_event m_events[MAXEVENTS];

    task_queue* m_task_queue{nullptr};
};

using task_callback_t = std::function<void(event_loop*)>;
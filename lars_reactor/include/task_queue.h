#pragma once

#include "task_msg.h"

#include <queue>
#include <mutex>

#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>
#include <sys/epoll.h> 

class task_queue {
public:
    task_queue();

    ~task_queue();

    event_loop* get_loop() const { return m_loop; }

    void deal_task();
    void set_loop(event_loop* loop);
    void add(const task_msg& task);
    void get(std::queue<task_msg>& queue);

private:
    event_loop* m_loop{nullptr};
    int m_efd{-1};
    std::queue<task_msg> m_queue;

    std::mutex m_mutex;
};
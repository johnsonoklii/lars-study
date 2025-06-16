#pragma once

#include "event_loop_thread_pool.h"

class store_thread_pool {
public:

    ~store_thread_pool() = default;

    static store_thread_pool& get_instance() {
        static store_thread_pool instance;
        return instance;
    }

    void start();
    void add_task(const task_msg& task);
private:
    store_thread_pool();
    event_loop_thread_pool::uptr m_loop_pool;
};
#pragma once

#include "event_loop_thread.h"

class event_loop_thread_pool {
public:
    using uptr = std::unique_ptr<event_loop_thread_pool>;
    event_loop_thread_pool(event_loop* loop);
    ~event_loop_thread_pool() = default;

    void set_thread_num(int thread_num) { m_thread_num = thread_num; }

    void start();

    event_loop* get_next_loop();
    std::vector<event_loop*> get_all_loop() { return m_loops; }
private:
    event_loop* m_main_loop{nullptr};
    int m_thread_num{0};
    std::vector<event_loop_thread::uptr> m_threads;
    std::vector<event_loop*> m_loops;
    int m_next{0};
};
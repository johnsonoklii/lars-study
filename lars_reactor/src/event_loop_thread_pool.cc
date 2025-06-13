#include "event_loop_thread_pool.h"

#include <stdio.h>

event_loop_thread_pool::event_loop_thread_pool(event_loop* loop)
: m_main_loop(loop) {
    m_threads.reserve(m_thread_num);
    m_loops.reserve(m_thread_num);
}

void event_loop_thread_pool::start() {
    for (int i = 0; i < m_thread_num; ++i) {
        printf("start thread %d\n", i);
        event_loop_thread* t = new event_loop_thread();
        event_loop_thread::uptr event_loop_uptr = std::unique_ptr<event_loop_thread>();
        m_threads.push_back(std::move(event_loop_uptr));
        m_loops.push_back(t->start());
    }
}

event_loop* event_loop_thread_pool::get_next_loop() {
    if (m_thread_num <= 0) {
        return m_main_loop;
    }
    
    if (m_next >= m_thread_num) {
        m_next = 0;
    }

    return m_loops[m_next++];
}
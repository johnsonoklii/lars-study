#include "event_loop_thread.h"

#include <assert.h>

event_loop* event_loop_thread::start() {
    m_thread = std::thread(std::bind(&event_loop_thread::thread_func, this));
    m_thread.detach();

    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_loop == nullptr) {
        m_cond.wait(lock);
    }

    assert(m_loop != nullptr);
    return m_loop;
}

void event_loop_thread::thread_func() {
    event_loop loop;

    m_loop = &loop;

    m_cond.notify_one();

    loop.loop();
}
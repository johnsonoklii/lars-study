#pragma once

#include "event_loop.h"

#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>

class event_loop_thread {
public:
    using uptr = std::unique_ptr<event_loop_thread>;
    event_loop_thread() = default;
    ~event_loop_thread() = default;

    event_loop* start();
    event_loop* get_loop() const { return m_loop; }
private:
    void thread_func();

private:
    event_loop* m_loop;

    std::thread m_thread;

    std::mutex m_mutex;
    std::condition_variable m_cond;
};
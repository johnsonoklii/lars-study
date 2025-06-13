#include "task_queue.h"
#include "event_loop.h"
#include <stdio.h>

task_queue::task_queue() {
    m_efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_efd < 0) {
        perror("eventfd");
        exit(1);
    }
}

task_queue::~task_queue() {
    if (m_efd >= 0) {
        ::close(m_efd);
    }
}

void task_queue::deal_task() {
    std::queue<task_msg> tasks;
    get(tasks);

    while (tasks.empty() != true) {
        auto task = tasks.front();
        tasks.pop();
        task.task_cb(m_loop);
    }
}

void task_queue::set_loop(event_loop* loop) {
    m_loop = loop;
    m_loop->add_io_event(m_efd, EPOLLIN, [this]() {
        deal_task();
    });
}

void task_queue::add(const task_msg& task) {
    unsigned long long idle_num = 1;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(task);
    }

    int ret = ::write(m_efd, &idle_num, sizeof(idle_num));
    if (ret < 0) {
        perror("m_efd write");
        exit(1);
    }
}

void task_queue::get(std::queue<task_msg>& queue) {
    unsigned int long long idle_num = 1;
    int ret = ::read(m_efd, &idle_num, sizeof(idle_num));
    if (ret < 0) {
        perror("m_efd read");
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::swap(queue, m_queue);
}
#include "event_loop.h"
#include "task_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

thread_local std::unordered_map<int, net_connection::sptr> t_event_loop_conns;

constexpr int TIMEOUT = 1000*10;

event_loop::event_loop() {
    m_efd = epoll_create1(0);
    if (m_efd < 0) {
        fprintf(stderr, "epoll_create error\n");
        exit(1);
    }

    m_task_queue = new task_queue();
    m_task_queue->set_loop(this);
}

event_loop::~event_loop() {
    if (m_efd > 0) {
        close(m_efd);
    }
    delete m_task_queue;
}

void event_loop::loop() {

    while (true) {
        int nfds = epoll_wait(m_efd, m_events, MAXEVENTS, TIMEOUT);
        if (nfds < 0) {
            fprintf(stderr, "epoll_wait error\n");
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = m_events[i].data.fd;
            int events = m_events[i].events;

            if (m_io_evs_map.find(fd) == m_io_evs_map.end()) {
                continue; // No such event
            }

            io_event ev = m_io_evs_map[fd]; // FIXME: 这里不要用io_event&，如果close删除掉，ev就析构了，出现段错误
            if (events & EPOLLIN && ev.m_read_callback) {
                ev.m_read_callback();
            }
            if (events & EPOLLOUT && ev.m_write_callback) {
                ev.m_write_callback();
            } 
            if (events & (EPOLLHUP|EPOLLERR)) {
                //水平触发未处理，可能会出现HUP事件，正常处理读写，没有则清空
                if (ev.m_read_callback == nullptr && ev.m_write_callback == nullptr) {
                    // 如果读写回调都没有，则删除事件
                    fprintf(stderr, "fd %d get error, delete it from epoll\n", fd);
                    del_io_event(fd);
                }

                if (ev.m_read_callback) {
                    ev.m_read_callback();
                }
                if (ev.m_write_callback) {
                    ev.m_write_callback();
                }
            }
        }
    
    }
}

void event_loop::add_io_event(int fd, int event, io_callback_t proc) {
    int final_event;
    int op;
    if (m_io_evs_map.find(fd) == m_io_evs_map.end()) {
        // New event
        final_event = event;
        op = EPOLL_CTL_ADD;
    } else {
        // Modify existing event
        final_event = m_io_evs_map[fd].m_event | event;
        op = EPOLL_CTL_MOD;
    }

    if (event & EPOLLIN) {
        // printf("add read event: EPOLLIN\n");
        m_io_evs_map[fd].m_read_callback = proc;
    } else if (event & EPOLLOUT) {
        // printf("msgid: EPOLLOUT\n");
        m_io_evs_map[fd].m_write_callback = proc;
    }

    m_io_evs_map[fd].m_event = final_event;

    struct epoll_event ev;
    ev.events = final_event;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_efd, op, fd, &ev); 
    if (ret < 0) {
        fprintf(stderr, "epoll ctl %d error\n", fd);
        return;
    }

    m_listen_fds.emplace(fd);
}

void event_loop::del_io_event(int fd) {
    if (m_io_evs_map.find(fd) == m_io_evs_map.end()) {
        return; // No such event
    }

    m_io_evs_map.erase(fd);
    m_listen_fds.erase(fd);

    int ret = epoll_ctl(m_efd, EPOLL_CTL_DEL, fd, nullptr);
    if (ret < 0) {
        fprintf(stderr, "epoll ctl delete %d error\n", fd);
        return;
    }

    return;
}

void event_loop::del_io_event(int fd, int event) {
    auto it = m_io_evs_map.find(fd);
    if (it == m_io_evs_map.end()) {
        return; // No such event
    }
    it->second.m_event = it->second.m_event & (~event);
    if (it->second.m_event == 0) {
        del_io_event(fd);
        return;
    }

    struct epoll_event ev;
    ev.events = it->second.m_event;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_efd, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0) {
        fprintf(stderr, "epoll ctl modify %d error\n", fd);
        return;
    }

    return;
}


void event_loop::add_task(const task_msg& task) {
    // TODO: 如果当前loop属于当前线程，可以直接执行task
    m_task_queue->add(task);
}



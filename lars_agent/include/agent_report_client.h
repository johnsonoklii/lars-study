#pragma once

#include "event_loop.h"
#include "event_loop_thread.h"

#include <stdio.h>

class report_client {
public:
    void start() {
        m_loop = m_thread.start();
        if (!m_loop) {
            fprintf(stderr, "report_client start failed\n");
            exit(-1);
        }
        printf("report_client start success\n");
    }

    void add_task(const task_msg& task) { m_loop->add_task(task); }
private:
    event_loop* m_loop;
    event_loop_thread m_thread;
};


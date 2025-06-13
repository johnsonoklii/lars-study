#pragma once

#include "event_loop.h"
#include <functional>

struct task_msg {
    enum class TASK_TYPE {
        kNEW_CONN,
        kNEW_TASK
    };

    TASK_TYPE type;
    task_callback_t task_cb;
};
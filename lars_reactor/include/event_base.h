#pragma once

#include <functional>

using io_callback_t = std::function<void()>;

struct io_event {
    int m_event{0}; // EPOLLIN, EPOLLOUT, etc.
    io_callback_t m_read_callback{nullptr};
    io_callback_t m_write_callback{nullptr};
};
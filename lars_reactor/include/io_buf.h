#pragma once

#include <stddef.h>

class io_buf {
public:
    io_buf(size_t cap);
    ~io_buf() = default;

    // 清空数据
    void clear();

    // 将已经处理过的数据，清空,将未处理的数据提前至数据首地址
    void adjust();

    // 将其他io_buf对象数据考到自己中
    void copy(const io_buf *other);

    // 处理长度为len的数据
    void pop(size_t len);

    bool is_avail() const { return m_length > 0; }

    char* m_data{nullptr};        // 缓冲区数据
    io_buf* m_next{nullptr};    // 链表组织缓存池
    size_t m_capacity{0};       // 缓冲区容量
    size_t m_length{0};         // 有效数据长度
    size_t m_head{0};           // 有效数据的起始位置
};
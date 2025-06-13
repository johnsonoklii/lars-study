#include "io_buf.h"

#include <assert.h>
#include <string.h>

io_buf::io_buf(size_t cap): m_capacity(cap) {
    m_data = new char[cap];
    assert(m_data);
}

void io_buf::clear() {
    m_head = 0;
    m_length = 0;
}

void io_buf::adjust() {
    if (m_head > 0) {
        if (m_length > 0) {
            memmove(m_data, m_data + m_head, m_length);
        }
        m_head = 0;
    }
}

void io_buf::copy(const io_buf *other) {
    memcpy(m_data, other->m_data + other->m_head, other->m_length);
    m_length = other->m_length;
    m_head = 0;
}

void io_buf::pop(size_t len) {
    m_head += len;
    m_length -= len;
}
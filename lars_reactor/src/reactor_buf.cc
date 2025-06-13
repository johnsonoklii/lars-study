#include "reactor_buf.h"
#include "buf_pool.h"

#include <assert.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

reactor_buf::reactor_buf() {
}

reactor_buf::~reactor_buf() {
    clear();
}

int reactor_buf::length() const {
    return m_buf ? m_buf->m_length : 0;
}

void reactor_buf::pop(int len) {
    assert(m_buf && len <= (int)m_buf->m_length);

    m_buf->pop(len);
    if (!m_buf->is_avail()) {
        buf_pool::get_instace().revert(m_buf);
        m_buf = nullptr;
    }
}

void reactor_buf::clear() {
    if (m_buf) {
        buf_pool::get_instace().revert(m_buf);
        m_buf = nullptr;
    }
}

//  input_buf
// TODO: 优化
int input_buf::read_data(int fd) {

    int need_read;//硬件有多少数据可以读

    //一次性读出所有的数据
    //需要给fd设置FIONREAD,
    //得到read缓冲中有多少数据是可以读取的
    if (ioctl(fd, FIONREAD, &need_read) == -1) {
        fprintf(stderr, "ioctl FIONREAD\n");
        return -1;
    }

    // 1. 没有buf，申请
    if (!m_buf) {
        m_buf = buf_pool::get_instace().alloc_buf(need_read);
        if (!m_buf) {
            fprintf(stderr, "alloc_buf failed\n");
            return -1;
        }
    }
    
    assert(m_buf->m_head == 0);

    // 2. 可用容量不够，重新分配
    if (int(m_buf->m_capacity - m_buf->m_length) < need_read) {
        io_buf* new_buf = buf_pool::get_instace().alloc_buf(m_buf->m_capacity + need_read);
        if (!new_buf) {
            fprintf(stderr, "alloc_buf failed\n");
            return -1;
        }
        new_buf->copy(m_buf);
        buf_pool::get_instace().revert(m_buf);
        m_buf = new_buf;
    }

    // 3. 读取数据到缓冲区
    int has_read = 0;
    do {
        if (need_read == 0) {
            // 可能是read阻塞读数据的模式，对方未写数据
            has_read = read(fd, m_buf->m_data + m_buf->m_length, m4K);
        } else {
            has_read = read(fd, m_buf->m_data + m_buf->m_length, need_read);
        }
       
    } while (has_read == -1 && errno == EINTR);

    if (has_read > 0) {
        if (need_read != 0) {
            assert(has_read == need_read);
        }

         m_buf->m_length += has_read;
    }

    return has_read;
}

const char* input_buf::data() const {
    return m_buf ? m_buf->m_data + m_buf->m_head : nullptr;
}

void input_buf::adjust() {
    if (m_buf) {
        m_buf->adjust();
    }
}

int output_buf::write_data(const char* data, int len) {
    // 1. 没有buf，申请
    if (!m_buf) {
        m_buf = buf_pool::get_instace().alloc_buf(len);
        if (!m_buf) {
            fprintf(stderr, "alloc_buf failed\n");
            return -1;
        }
    }

    // 2. 可用容量不够，重新分配
    if (int(m_buf->m_capacity - m_buf->m_length) < len) {
        io_buf* new_buf = buf_pool::get_instace().alloc_buf(m_buf->m_capacity + len);
        if (!new_buf) {
            fprintf(stderr, "alloc_buf failed\n");
            return -1;
        }
        new_buf->copy(m_buf);
        buf_pool::get_instace().revert(m_buf);
        m_buf = new_buf;
    }

    // 3. 复制
    memcpy(m_buf->m_data + m_buf->m_length, data, len);
    m_buf->m_length += len;

    return len;
}

// TODO: 优化
int output_buf::write2fd(int fd) {
    assert(m_buf && m_buf->m_head == 0);

    int has_write = 0;
    do {
        has_write = write(fd, m_buf->m_data, m_buf->m_length);
    } while (has_write == -1 && errno == EINTR);

    if (has_write > 0) {
        m_buf->pop(has_write);
        m_buf->adjust();
    }

    if (has_write == -1 && (errno == EAGAIN ||  errno == EWOULDBLOCK)) {
        has_write = 0;
    }

    return has_write;
}
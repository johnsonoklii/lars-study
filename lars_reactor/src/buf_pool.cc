#include "buf_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

buf_pool::buf_pool() {
    // 默认创建一些io_buf
    io_buf* prev; 

    // 4K的io_buf 预先开辟5000个，约20MB供开发者使用
    m_pool[m4K] = new io_buf(m4K);
    if (m_pool[m4K] == nullptr) {
        fprintf(stderr, "new io_buf m4K error\n");
        exit(1);
    }

    prev =  m_pool[m4K];
    for (int i = 0; i < 5000; ++i) {
        prev->m_next = new io_buf(m4K);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m4K error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 5000 * 4;

    // 16K的io_buf 预先开辟1000个，约16MB供开发者使用
    m_pool[m16K] = new io_buf(m16K);
    if (m_pool[m16K] == nullptr) {
        fprintf(stderr, "new io_buf m16K error\n");
        exit(1);
    }

    prev =  m_pool[m16K];
    for (int i = 0; i < 1000; ++i) {
        prev->m_next = new io_buf(m16K);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m16K error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 1000 * 16;

    // 64K的io_buf 预先开辟500个，约32MB供开发者使用
    m_pool[m64K] = new io_buf(m64K);
    if (m_pool[m64K] == nullptr) {
        fprintf(stderr, "new io_buf m64K error\n");
        exit(1);
    }

    prev =  m_pool[m64K];
    for (int i = 0; i < 500; ++i) {
        prev->m_next = new io_buf(m64K);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m64K error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 500 * 64;

    // m256K的io_buf 预先开辟200个，约50MB供开发者使用
    m_pool[m256K] = new io_buf(m256K);
    if (m_pool[m256K] == nullptr) {
        fprintf(stderr, "new io_buf m256K error\n");
        exit(1);
    }

    prev =  m_pool[m256K];
    for (int i = 0; i < 200; ++i) {
        prev->m_next = new io_buf(m256K);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m256K error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 200 * 256;

    // m1M的io_buf 预先开辟50个，约50MB供开发者使用
    m_pool[m1M] = new io_buf(m1M);
    if (m_pool[m1M] == nullptr) {
        fprintf(stderr, "new io_buf m1M error\n");
        exit(1);
    }

    prev =  m_pool[m1M];
    for (int i = 0; i < 50; ++i) {
        prev->m_next = new io_buf(m1M);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m1M error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 50 * 1024;

    // m4M的io_buf 预先开辟20个，约80MB供开发者使用
    m_pool[m4M] = new io_buf(m4M);
    if (m_pool[m4M] == nullptr) {
        fprintf(stderr, "new io_buf m4M error\n");
        exit(1);
    }

    prev =  m_pool[m4M];
    for (int i = 0; i < 20; ++i) {
        prev->m_next = new io_buf(m4M);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m4M error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 20 * 4096;

    // m8M的io_buf 预先开辟10个，约80MB供开发者使用
    m_pool[m8M] = new io_buf(m8M);
    if (m_pool[m8M] == nullptr) {
        fprintf(stderr, "new io_buf m8M error\n");
        exit(1);
    }

    prev =  m_pool[m8M];
    for (int i = 0; i < 20; ++i) {
        prev->m_next = new io_buf(m8M);
        if (prev->m_next == nullptr) {
            fprintf(stderr, "new io_buf m8M error\n");
            exit(1);
        }
        prev = prev->m_next;
    }
    m_total_mem += 10 * 8192;
}

buf_pool::~buf_pool() {
    // TODO 释放内存
}

io_buf* buf_pool::alloc_buf(int N) {
    if (N < 0) {
        return nullptr;
    }
    // 1. 找到第一个 > N 的index
    int index = 0;
    if (N <= m4K) {
        index = m4K;
    } else if (N <= m64K) {
        index = m64K;
    } else if (N <= m256K) {
        index = m256K;
    } else if (N <= m1M) {
        index = m1M;
    } else if (N <= m4M) {
        index = m4M;
    } else if (N <= m8M) {
        index = m8M;
    }else {
        return nullptr;
    }

    // 2. 从缓存池获取一个io_buf
    std::lock_guard<std::mutex> lock(m_mutex);
    io_buf* ret_buf = m_pool[index];
    if (ret_buf == nullptr) {
        // 没有可用的，创建一个
        if (m_total_mem + index/1024 > EXTRA_MEM_LIMIT) {
            fprintf(stderr, "buf_pool memory limit exceeded\n");
            exit(1);
        }
        io_buf* new_buf = new io_buf(index);
        if (ret_buf == nullptr) {
            fprintf(stderr, "new io_buf error\n");
            exit(1);
        }
        m_total_mem += index / 1024; // 更新总内存
        return new_buf;
    }

    m_pool[index] = ret_buf->m_next;
    ret_buf->m_next = nullptr;
    return ret_buf;
}

void buf_pool::revert(io_buf *buf) {
    buf->clear();
    int index = buf->m_capacity;

    std::lock_guard<std::mutex> lock(m_mutex);

    assert(m_pool.find(index) != m_pool.end());
    buf->m_next = m_pool[index];
    m_pool[index] = buf;
}
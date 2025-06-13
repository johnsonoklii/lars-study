#pragma once

#include "io_buf.h"

#include <stddef.h>
#include <stdint.h>
#include <unordered_map>

#include <mutex>

enum MEM_CAP {
    m4K     = 4096,
    m16K    = 16384,
    m64K    = 65536,
    m256K   = 262144,
    m1M     = 1048576,
    m4M     = 4194304,
    m8M     = 8388608
};

// 总内存池最大限制 单位是Kb 所以目前限制是 5GB
constexpr size_t EXTRA_MEM_LIMIT = 5U *1024 *1024;

// key: MEM_CAP, value: io_buf list
using pool_t = std::unordered_map<size_t, io_buf*>;

class buf_pool {
public:
    ~buf_pool();
    static buf_pool& get_instace() {
        static buf_pool instance;
        return instance;
    }

    // 申请一个buf
    io_buf *alloc_buf(int N);
    io_buf *alloc_buf() { return alloc_buf(m4K); }

    // 放回一个buf
    void revert(io_buf *buf);
    
private:
    buf_pool();
    buf_pool(const buf_pool&);
    const buf_pool& operator=(const buf_pool&);

    pool_t m_pool;
    uint64_t m_total_mem{0};
    
    std::mutex m_mutex;
};
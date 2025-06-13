#pragma once

#include "io_buf.h"

// 给业务层最后使用的
class reactor_buf {
public:
    reactor_buf();
    ~reactor_buf();

    int length() const;
    void pop(int len);
    void clear();
protected:
    io_buf* m_buf{nullptr};
};

class input_buf: public reactor_buf {
public:
    int read_data(int fd);
    const char* data() const;
    void adjust();
};

class output_buf: public reactor_buf {
public:
    int write_data(const char* data, int len);
    int write2fd(int fd);
};
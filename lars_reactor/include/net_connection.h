#pragma once

#include <memory>

class net_connection {
public:
    using sptr = std::shared_ptr<net_connection>;
    virtual ~net_connection() = default;
	virtual int send(int msgid, const char* data, int len) = 0;

    virtual int fd() = 0;

    void set_context(void *ctx) { context = ctx; }
    void* get_context() { return context; }
    void* context; // context
};
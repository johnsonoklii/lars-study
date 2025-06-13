#pragma once

#include "tcp_conn.h"

#include <functional>
#include <stdint.h>
#include <unordered_map>

using handler_t = std::function<void(net_connection::sptr, int msgid, const char* data, int len)>;

class handler_map  {
public:
    static handler_map& get_instance() {
        static handler_map instance;
        return instance;
    }

    void register_handler(int msgid, handler_t handler) {
        m_handler_map[msgid] = handler;
    }

    handler_t get_handler(int msgid) {
        auto it = m_handler_map.find(msgid);
        if (it != m_handler_map.end()) {
            return it->second;
        }
        return nullptr;
    }
    
private:
    handler_map() = default;
    std::unordered_map<int, handler_t> m_handler_map;
};
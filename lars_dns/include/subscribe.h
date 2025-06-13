#pragma once

#include "lars_reactor.h"

#include <stdint.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

using publish_map = std::unordered_map<int, std::unordered_set<uint64_t>>;   // fd : modid/cmdid set
using subscribe_map = std::unordered_map<int, std::unordered_set<uint64_t>>; // modid/cmdid : fd set

class subscribe_list {
public:
    // subscribe_list() = default;
    subscribe_list(const subscribe_list &) = delete;
    subscribe_list& operator=(const subscribe_list &) = delete;

    static subscribe_list& get_instance() {
        static subscribe_list instance;
        return instance;
    }

    //订阅
    void subscribe(int fd, uint64_t mod);
    
    //取消订阅
    void unsubscribe(int fd, uint64_t mod);

    //发布
    void publish(std::vector<uint64_t> &change_mods);

    //根据在线用户fd得到需要发布的列表
    void make_publish_map(listen_fd_set &online_fds, publish_map &need_publish);
private:
    subscribe_list() = default;

    void push_change_task(event_loop *loop);

private:
    std::mutex m_subscribe_map_mutex;
    std::mutex m_publish_map_mutex;

    subscribe_map m_subscribe_map;
    publish_map m_publish_map;      //fd： 待发送的消息
};
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

    // 这些函数都会当作任务发送到loop的线程，因此是线程安全的
    //订阅
    static void subscribe(int fd, uint64_t mod);
    
    //取消订阅
    static void unsubscribe(int fd, uint64_t mod);

    //发布
    static void publish(event_loop* loop, std::vector<uint64_t> &change_mods);

    //根据在线用户fd得到需要发布的列表
    static void make_publish_map(listen_fd_set &online_fds, publish_map &need_publish);
private:
    subscribe_list() = default;

    static void push_change_task(event_loop *loop);

private:
    static thread_local subscribe_map s_t_subscribe_map;
    static thread_local publish_map s_t_publish_map;      //fd： 待发送的消息
};
#pragma once

#include "host_info.h"

#include <unordered_map>
#include <list>

#include <stdint.h>

class load_balance {
public:
    load_balance(int modid, int cmdid);
    ~load_balance();

    bool empty() const { return m_host_map.empty(); }

    // 从当前的双队列中获取host信息
    int choice_one_host(host_info& rsp);

    //如果list中没有host信息，需要从远程的DNS Service发送GetRouteHost请求申请
    int pull();

    //根据dns service远程返回的结果，更新_host_map
    void update(std::vector<host_info>& host_infos);

    //当前load_balance模块的状态
    enum STATUS
    {
        PULLING, //正在从远程dns service通过网络拉取
        NEW      //正在创建新的load_balance模块
    };
    STATUS status;  //当前的状态

private:
    int m_modid;
    int m_cmdid;
    int m_access_cnt;    //请求次数，每次请求+1,判断是否超过probe_num阈值

    std::unordered_map<uint64_t, host_info> m_host_map; // ip + port : host_info
    std::list<host_info> m_idle_list;
    std::list<host_info> m_busy_list;
};
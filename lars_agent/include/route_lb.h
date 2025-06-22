#pragma once

#include "load_balance.h"

#include "lars.pb.h"

class route_lb {
public:
    route_lb(int id);

    //agent获取一个host主机
    int get_host(int modid, int cmdid, host_info& host);

    //根据Dns Service返回的结果更新自己的route_lb_map
    int update_host(int modid, int cmdid, std::vector<host_info>& host_infos);

private:
    int m_id;

    std::unordered_map<uint64_t, load_balance*> m_lb_map; // mod_cmd : load_balance
};
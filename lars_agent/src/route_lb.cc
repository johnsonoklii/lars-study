#include "route_lb.h"
#include "host_info.h"

#include <assert.h>

route_lb::route_lb(int id): m_id(id) {

}

//agent获取一个host主机
int route_lb::get_host(int modid, int cmdid, host_info& host) {
    // TODO: 加锁   
    uint64_t mod_cmd = ((uint64_t)modid << 32) + cmdid;

    auto it = m_lb_map.find(mod_cmd);
    if (it == m_lb_map.end()) {
        // 没有这个mod_cmd, 创建load_balance
        m_lb_map[mod_cmd] = new load_balance(modid, cmdid);
        m_lb_map[mod_cmd]->pull();

        return lars::RET_NOEXIST;
    }

    auto lb = it->second;
    if (lb->empty()) {
        // load_balance为空, 重新拉取
        //存在lb 里面的host为空，说明正在pull()中，还没有从dns_service返回来,所以直接回复不存在
        assert(lb->status == load_balance::PULLING);
        return lars::RET_NOEXIST;
    }

    return lb->choice_one_host(host);
}

//根据Dns Service返回的结果更新自己的route_lb_map
int route_lb::update_host(int modid, int cmdid, std::vector<host_info>& host_infos) {
    // TODO: 加锁
    (void)modid; (void)cmdid; (void)host;

    uint64_t mod_cmd = ((uint64_t)modid << 32) + cmdid;

    auto it = m_lb_map.find(mod_cmd);
    if (it == m_lb_map.end()) {
        // 没有这个mod_cmd, 创建load_balance
        m_lb_map[mod_cmd] = new load_balance(modid, cmdid);
        m_lb_map[mod_cmd]->pull();
        return 0;
    }

    auto lb = it->second;
    if (host_infos.empty()) {
        delete lb;
        m_lb_map.erase(it);
    } else {
        lb->update(host_infos);
    }
    
    return 0;
}
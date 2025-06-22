#include "load_balance.h"
#include "agent_dns_client.h"
#include "task_msg.h"

#include "lars.pb.h"

static void get_host_from_list(host_info& h_info, std::list<host_info>& l) {
    if (l.empty()) {
        return;
    }

    auto it = l.front();
    l.pop_front();

    h_info.ip = it.ip;
    h_info.port = it.port;

    l.push_back(it);
}

load_balance::load_balance(int modid, int cmdid)
: m_modid(modid), m_cmdid(cmdid) {

}

load_balance::~load_balance() {

}

int load_balance::choice_one_host(host_info& h_info) {

    if (m_idle_list.empty()) {
        // 给busy_list一个机会
        if (m_access_cnt >= 10) {
            m_access_cnt = 0;
            get_host_from_list(h_info, m_busy_list);
            return lars::RET_SUCC;
        } else {
            // 过载
            ++m_access_cnt;
            return lars::RET_OVERLOAD; 
        }
    } 

    if (!m_busy_list.empty() && m_access_cnt >= 10) {
        m_access_cnt = 0;
        get_host_from_list(h_info, m_busy_list);
        return lars::RET_SUCC;
    }

    ++m_access_cnt;
    get_host_from_list(h_info, m_idle_list);
    return lars::RET_SUCC;
}

int load_balance::pull() {
    // 1. dns client 发送请求
    task_msg task;
    task.type = task_msg::TASK_TYPE::kNEW_TASK;
    task.task_cb = [this](event_loop*) {
        lars::GetHostRequest req;
        req.set_modid(m_modid);
        req.set_cmdid(m_cmdid);

        std::string req_str = req.SerializeAsString();
        g_dns_client.send(lars::ID_GetHostRequest, req_str.c_str(), req_str.size());
    };

    g_dns_client.add_task(task);

    status = PULLING;  

    return 0;
}

void load_balance::update(std::vector<host_info>& host_infos) {
    
    
    
}  

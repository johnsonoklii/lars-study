#include "subscribe.h"
#include "dns_route.h"

#include "lars.pb.h"

extern tcp_server* server;

// TODO: 1. subscribe_list改成thread_loacl，属于一个loop，防止加锁

void subscribe_list::subscribe(int fd, uint64_t mod) {
    std::lock_guard<std::mutex> lock(m_subscribe_map_mutex);
    m_subscribe_map[mod].emplace(fd);
}

void subscribe_list::unsubscribe(int fd, uint64_t mod) {
    std::lock_guard<std::mutex> lock(m_subscribe_map_mutex);
    m_subscribe_map[mod].erase(fd);
    if (m_subscribe_map[mod].empty()) {
        m_subscribe_map.erase(mod);
    }
}

void subscribe_list::publish(std::vector<uint64_t> &change_mods) {
    {
        std::lock_guard<std::mutex> lock1(m_subscribe_map_mutex);
        std::lock_guard<std::mutex> lock2(m_publish_map_mutex);

        for (auto mod : change_mods) {
            if (m_subscribe_map.find(mod) != m_subscribe_map.end()) {
                auto fds = m_subscribe_map[mod];
                // 为了减少发送频率，先将fd要发送的消息整合在一起
                for (auto fd : fds) {
                    m_publish_map[fd].emplace(mod);
                }
            }
        }
    }

    server->send_task(std::bind(&subscribe_list::push_change_task, this, std::placeholders::_1));
}

void subscribe_list::make_publish_map(listen_fd_set &online_fds, publish_map &need_publish) {
    std::lock_guard<std::mutex> lock(m_publish_map_mutex);

    for (auto fd : online_fds) {
        if (m_publish_map.find(fd) != m_publish_map.end()) {
            need_publish.emplace(fd, m_publish_map[fd]);
            m_publish_map.erase(fd);
        }
    }
}


void subscribe_list::push_change_task(event_loop *loop) {
    listen_fd_set online_fds = loop->get_listen_fds();

    // 当前loop需要pushlish的map
    publish_map need_publish_map;
    make_publish_map(online_fds, need_publish_map);

    printf("push_change_task\n");

    for (auto it : need_publish_map) {
        int fd = it.first;
        printf("fd=%d\n", fd);
        // TODO: 2. loop应该有一个thread_local的conns
        net_connection::sptr conn = t_event_loop_conns[fd];
        std::unordered_set<uint64_t>& need_pushlish = it.second;
        for (auto modcmd : need_pushlish) {
            int modid = modcmd >> 32;
            int cmdid = (uint32_t)modcmd;
            
            lars::GetRouteResponse rsp;
            rsp.set_modid(modid);  
            rsp.set_cmdid(cmdid);

            const host_set& hosts = route::get_instance().get_hosts(modid, cmdid);
            for (auto host : hosts) {
                lars::HostInfo host_info;
                host_info.set_ip(host >> 32);
                host_info.set_port((uint32_t)host);
                rsp.add_host()->CopyFrom(host_info);
            }

            std::string rsp_str = rsp.SerializeAsString();
            conn->send(lars::MessageId::ID_GetRouteResponse, rsp_str.c_str(), rsp_str.size());
        }
    }
}
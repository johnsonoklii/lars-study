#pragma once

#include "tcp_client.h"
#include "event_loop.h"
#include "event_loop_thread.h"
#include "config_file.h"
#include "route_lb.h"

#include <stdio.h>

extern std::vector<route_lb*> g_route_lbs;

class dns_client {
public:
    dns_client()= default;

    void start() {
        m_loop = m_thread.start();
        if (!m_loop) {
            fprintf(stderr, "dns_client start failed\n");
            exit(-1);
        }

        std::string ip = config_file::instance()->GetString("dnsserver", "ip", "");
        short port = config_file::instance()->GetNumber("dnsserver", "port", 0);
        
        //2 创建客户端
        m_client = std::make_shared<tcp_client>(m_loop, ip.c_str(), port);
        m_client->add_router(lars::ID_GetRouteResponse, std::bind(&dns_client::deal_recv_route, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        m_client->do_connect();

        printf("dns_client start success\n");
    }

    void deal_recv_route(net_connection::sptr, int msgid, const char* data, int len) {
        (void)msgid;

        lars::GetRouteResponse rsp;
        rsp.ParseFromArray(data, len);
        int modid = rsp.modid();
        int cmdid = rsp.cmdid();
        int index = (modid+cmdid)%3;

        host_info host;
        
        // 将该modid/cmdid交给一个route_lb处理 将rsp中的hostinfo集合加入到对应的route_lb中
        g_route_lbs[index]->update_host(modid, cmdid, host);
    }

    void add_task(const task_msg& task) { 
        printf("dns_client add task...\n");
        m_loop->add_task(task); 
    }

    int send(int msgid, const char* data, int len) {
        return m_client->send(msgid, data, len);
    }
    
private:
    event_loop* m_loop{nullptr};
    tcp_client::sptr m_client{nullptr};
    event_loop_thread m_thread;
};


extern dns_client g_dns_client;
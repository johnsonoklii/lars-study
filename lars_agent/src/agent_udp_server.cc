#include "agent_server.h"
#include "agent_dns_client.h"
#include "agent_report_client.h"
#include "lars_reactor.h"
#include "route_lb.h"
#include "host_info.h"

#include "lars.pb.h"

#include <thread>

extern report_client s_g_report_client;
extern dns_client s_g_dns_client;

extern std::vector<route_lb*> g_route_lbs;

void get_host_handler(route_lb* r_lb, net_connection::sptr conn, int msgid, const char* data, int len) {
    (void)msgid;
    // 1. 解析
    lars::GetHostRequest req;
    req.ParseFromArray(data, len);
    int modid = req.modid();
    int cmdid = req.cmdid();
    int seq = req.seq();

    // 2. 逻辑处理
    lars::HostInfo host;
    host_info h_info;
    int ret = r_lb->get_host(modid, cmdid, h_info);
    if (ret == lars::RET_SUCC) {
        host.set_ip(h_info.ip);
        host.set_port(h_info.port);
    } else {
        printf("get host failed, modid=%d, cmdid=%d, ret_code=%d\n", modid, cmdid, ret);
    }

    // 3. 响应
    lars::GetHostResponse resp;
    resp.set_modid(modid);
    resp.set_cmdid(cmdid);
    resp.set_seq(seq);
    resp.set_retcode(ret);
    resp.mutable_host()->CopyFrom(host);

    std::string resp_str;
    resp.SerializeToString(&resp_str);
    conn->send(lars::ID_GetHostResponse, resp_str.c_str(), resp_str.size());
}

void run_udp_server(int i) {
    short port = i + 8888;

    event_loop loop;
    udp_server::sptr server = std::make_shared<udp_server>(&loop, "0.0.0.0", port);

    // TODO: 注册handler
    server->add_router(lars::ID_GetHostRequest, std::bind(get_host_handler, g_route_lbs[i], std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    printf("agent UDP server :port %d is started...\n", port);

    loop.loop();
}

void start_UDP_servers() {
    for (int i = 0; i < 3; ++i) {
        std::thread t(run_udp_server, i);
        t.detach();
    }
}
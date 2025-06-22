#include "agent_server.h"
#include "route_lb.h"
#include "config_file.h"

#include <unistd.h>
#include <iostream>
#include <vector>

std::vector<route_lb*> g_route_lbs(3);

static void create_route_lb() {
    for (int i = 0; i < 3; i++) {
        int id = i + 1; //route_lb的id从1 开始计数
        g_route_lbs[i]  = new route_lb(id);
        if (g_route_lbs[i] == NULL) {
            fprintf(stderr, "no more space to new route_lb\n");
            exit(1);
        }
    }
}

static void init_lb_agent() {
    //1. 加载配置文件
    config_file::setPath("./conf/lars_lb_agent.conf");
    // lb_config.probe_num = config_file::instance()->GetNumber("loadbalance", "probe_num", 10);
    // lb_config.init_succ_cnt = config_file::instance()->GetNumber("loadbalance", "init_succ_cnt", 180);

    //2. 初始化3个route_lb模块
    create_route_lb();
}


int main() {

    init_lb_agent();
    
    start_UDP_servers();
    sleep(1);

    start_report_client();

    start_dns_client();

    std::cout <<"done!" <<std::endl;

    while (1) {
        sleep(10);
    }

    return 0;
}
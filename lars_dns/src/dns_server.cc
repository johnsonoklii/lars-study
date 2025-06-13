#include "lars_reactor.h"
#include "dns_route.h"
#include "subscribe.h"

#include "lars.pb.h"

#include <mysql/mysql.h>

tcp_server* server;

void get_route(net_connection::sptr conn, int msgid, const char* data, int len);

class dns_server {
public:
    dns_server() {
        m_loop = new event_loop();
        std::string ip = config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
        int port = config_file::instance()->GetNumber("reactor", "port", 7778);
        int thread_num = config_file::instance()->GetNumber("reactor", "threadNum", 3);

        server = new tcp_server(m_loop, ip.c_str(), port);
        server->set_thraed_num(thread_num);

        // 注册handler
        server->add_router(lars::MessageId::ID_GetRouteRequest, get_route);

        server->set_on_build_callback(std::bind(&dns_server::on_build, this, std::placeholders::_1));
    }

    ~dns_server() {
        delete server;
        delete m_loop;
    }

    void on_build(const net_connection::sptr& conn) {
        printf("new conn: %d\n", conn->fd());
        uint64_t key = ((uint64_t)1 << 32) + 2; 
        subscribe_list::subscribe(conn->fd(), key);
        printf("subscribe modid = %d, cmdid = %d\n",1, 2);
    }

    void start() {
        // 初始化route
        route::get_instance();

        server->start();
        printf("lars dns service ....\n");
        m_loop->loop();
    }

private:
    event_loop* m_loop;
    // static thread_local subscribe_list s_t_subscribe_list;
};

// thread_local subscribe_list dns_server::s_t_subscribe_list;

void get_route(net_connection::sptr conn, int msgid, const char* data, int len) {
    (void)msgid;
    //1. 解析proto文件
    lars::GetRouteRequest req;
    req.ParseFromArray(data, len);

    int modid, cmdid;

    // 2. 业务逻辑
    modid = req.modid();
    cmdid = req.cmdid();
    host_set hosts = route::get_instance().get_hosts(modid, cmdid);

    // 3. 返回
    lars::GetRouteResponse resp;
    resp.set_modid(modid);
    resp.set_cmdid(cmdid);

    for (auto ip_port : hosts) {
        lars::HostInfo host;
        host.set_ip((uint32_t)(ip_port >> 32));
        host.set_port((int)(ip_port));
        resp.add_host()->CopyFrom(host);
    }

    std::string res_str = resp.SerializeAsString();

    conn->send(lars::MessageId::ID_GetRouteResponse, res_str.c_str(), res_str.size());
}

int main() {
    config_file::setPath("conf/lars_dns.conf");
    dns_server dns;
    dns.start();
}
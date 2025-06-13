#include <string.h>
#include <unistd.h>
#include <string>
#include "lars_reactor.h"
#include "lars.pb.h"

struct Option {
    Option():ip(NULL),port(0) {}
    const char* ip;
    int port;
};

void Usage() {
    printf("Usage: ./lars_dns_test1 -h ip -p port\n");
}

Option option;

void parse_option(int argc, char** argv) {
for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
            option.ip = argv[i + 1];
        }
        else if (strcmp(argv[i], "-p") == 0) {
            option.port = atoi(argv[i + 1]);
        }
    }

    if ( !option.ip || !option.port ) {
        Usage();
        exit(1);
    }
}

void deal_get_route(net_connection::sptr conn, int msgid, const char* data, int len) {
    (void)msgid;
    lars::GetRouteResponse resp;
    resp.ParseFromArray(data, len);

    printf("modid: %d, cmdid: %d, hosts.size: %d\n", resp.modid(), resp.cmdid(), resp.host_size());
    for (int i = 0; i < resp.host_size(); ++i) {
        printf("ip = %u, port = %d\n", resp.host(i).ip(), resp.host(i).port());
    }

    (void)conn;

    // 再请求
    // lars::GetRouteRequest req;
    // req.set_modid(resp.modid());
    // req.set_cmdid(resp.cmdid());

    // std::string req_str = req.SerializeAsString();

    // conn->send(lars::MessageId::ID_GetRouteRequest, req_str.c_str(), req_str.size());
}

void on_build(const net_connection::sptr& conn) {
    lars::GetRouteRequest req;
    req.set_modid(1);
    req.set_cmdid(2);

    std::string req_str = req.SerializeAsString();

    conn->send(lars::MessageId::ID_GetRouteRequest, req_str.c_str(), req_str.size());
}

int main(int argc, char** argv) {
    parse_option(argc, argv);

    event_loop loop;

    printf("ip = %s, port = %d\n", option.ip, option.port);

    // FIXME: 不是sptr，server会core dump
    tcp_client::sptr client = std::make_shared<tcp_client>(&loop, option.ip, option.port);
    client->set_on_build_callback(on_build);

    client->add_router(lars::MessageId::ID_GetRouteResponse, deal_get_route);
    client->do_connect();

    loop.loop();

    return 0 ;
}
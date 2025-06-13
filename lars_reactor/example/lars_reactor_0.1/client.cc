#include "tcp_client.h"
#include "handler.h"

#include <string>
#include <iostream>

void biz_echo(net_connection::sptr conn, int msgid, const char* data, int len) {
    (void)conn;
    // 1. TODO: 解析data为pb

    // 2. 业务处理
    std::string msg(data, len);
    std::cout << "recv msgid: "<< msgid << ", recv msg: " << msg << std::endl;
}

void register_handler() {
    handler_map::get_instance().register_handler(1, biz_echo);
}

void on_build(net_connection::sptr conn) {
    (void)conn;
    printf("client on_build\n");
}

void on_close(net_connection::sptr conn) {
    (void)conn;
    printf("client on_close\n");
}

int main() {
    register_handler();
    event_loop loop;
    tcp_client::sptr client = std::make_shared<tcp_client>(&loop, "127.0.0.1", 7777);
    client->set_on_build_callback(on_build);
    client->set_on_close_callback(on_close);
    client->do_connect();

    loop.loop();
    return 0;
}
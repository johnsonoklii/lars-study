#include "tcp_server.h"
#include "handler.h"
#include "config_file.h"

#include <string.h>
#include <string>
#include <unordered_set>

void biz_echo(net_connection::sptr conn, int msgid, const char* data, int len) {
    (void)data; (void)len;
    // 1. TODO: 解析data为pb

    // 2. 业务处理
    std::string  data_str(data, len);
    printf("msgid:%d, data:%s\n", msgid, data_str.c_str());


    // 3. 返回结果
    std::string res_str = "this is lars";
    conn->send(msgid, res_str.c_str(), res_str.size());
}

// 分发给其他线程的任务
void print_lars_task(event_loop *loop) {
    printf("======= Active Task Func! ========\n");
    (void)loop;

    for (auto it = t_event_loop_conns.begin(); it != t_event_loop_conns.end(); it++) {
        net_connection::sptr conn = it->second;
        if (conn != nullptr) {
            int msgid = 1;
            void* ctx = conn->get_context();
            const char* msg = (const char*)ctx;
            conn->send(msgid, msg, strlen(msg));
        }
    }
}

void register_handler() {
    handler_map::get_instance().register_handler(1, biz_echo);
}

void on_build(const net_connection::sptr& conn, tcp_server *server) {
    (void)server;
    int msgid = 1;
    const char *msg = "welcome! you online..";

    conn->send(msgid, msg, strlen(msg));

    const char *conn_param_test = "I am the conn for you!";
    conn->set_context((void*)conn_param_test);

    // 发送给所有loop
    server->send_task(std::bind(print_lars_task, std::placeholders::_1));
}

void on_close(net_connection::sptr conn) {
    (void)conn;
    printf("server on_close\n");
}

int main() {
    register_handler();
    event_loop loop;

    config_file::setPath("./serv.conf");
    std::string ip = config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
    int port = config_file::instance()->GetNumber("reactor", "port", 7777);
    int thread_num = config_file::instance()->GetNumber("reactor", "threadNum", 3);

    tcp_server server(&loop, ip.c_str(), port);
    server.set_on_build_callback(std::bind(&on_build, std::placeholders::_1, &server));
    server.set_on_close_callback(on_close);
    server.set_thraed_num(thread_num);
    server.start();

    loop.loop();
}
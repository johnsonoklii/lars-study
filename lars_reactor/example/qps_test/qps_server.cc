#include "lars_reactor.h"

#include "echoMessage.pb.h"

void biz_echo(net_connection::sptr conn, int msgid, const char* data, int len) {
    qps_test::EchoMessage request, response;  

    // 1. 解包
    request.ParseFromArray(data, len); 

    // 2. 构造响应包
    response.set_id(request.id());
    response.set_content(request.content());

    // 3. 序列化
    std::string responseString;
    response.SerializeToString(&responseString);


    // 3. 发送响应包
    conn->send(msgid, responseString.c_str(), responseString.size());
}


int main() {
    event_loop loop;
    config_file::setPath("./serv.conf");
    std::string ip = config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
    int port = config_file::instance()->GetNumber("reactor", "port", 7777);

    tcp_server server(&loop, ip.c_str(), port);

    // 注册路由
    server.add_router(1, biz_echo);

    loop.loop();
    return 0;
}
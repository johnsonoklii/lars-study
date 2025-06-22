#include <string>
#include <string.h>
#include "config_file.h"
#include "udp_server.h"

//回显业务的回调函数
void callback_busi(net_connection::sptr conn, int msgid, const char *data, uint32_t len)
{
    printf("callback_busi ...\n");
    //直接回显
    conn->send(msgid, data, len);
}

int main() 
{
    event_loop loop;

    //加载配置文件
    config_file::setPath("./serv.conf");
    std::string ip = config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
    short port = config_file::instance()->GetNumber("reactor", "port", 8888);

    printf("ip = %s, port = %d\n", ip.c_str(), port);

    udp_server::sptr server = std::make_shared<udp_server>(&loop, ip.c_str(), port);

    //注册消息业务路由
    server->add_router(1, callback_busi);

    loop.loop();

    return 0;
}
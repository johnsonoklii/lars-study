#include <stdio.h>
#include <string.h>

#include "udp_client.h"


//客户端业务
void busi(net_connection::sptr conn, int msgid, const char *data, uint32_t len)
{
    (void)conn;
    //得到服务端回执的数据 
    char *str = nullptr;
    
    str = (char*)malloc(len+1);
    memset(str, 0, len+1);
    memcpy(str, data, len);
    printf("recv server: [%s]\n", str);
    printf("msgid: [%d]\n", msgid);
    printf("len: [%d]\n", len);
}


int main() 
{
    event_loop loop;

    //创建udp客户端
    udp_client::sptr client = std::make_shared<udp_client>(&loop, "127.0.0.1", 7777);


    //注册消息路由业务
    client->add_router(1, busi);

    //发消息
    int msgid = 1; 
    const char *msg = "Hello Lars!";

    client->send(msgid, msg, strlen(msg));

    //开启事件监听
    loop.loop();

    return 0;
}
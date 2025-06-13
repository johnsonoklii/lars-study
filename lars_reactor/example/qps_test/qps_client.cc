#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>

#include "tcp_client.h"
#include "echoMessage.pb.h"

struct Qps
{
    Qps() {
        last_time = time(NULL); 
        succ_cnt = 0;
    }

    long last_time;//最后一次发包时间 ms为单位
    int succ_cnt; //成功收到服务器回显的次数
};


//客户端业务
void busi(net_connection::sptr conn, int msgid, const char* data, int len, Qps* qps)
{
    qps_test::EchoMessage request, response;

    //解析服务端传来的pb数据
    if (response.ParseFromArray(data, len) == false) {
        printf("server call back data error\n");
        return;
    }

    //判断数据内容是否回显一致
    if (response.content() == "Hello Lars!!!") {
        //服务器请求响应成功一次
        qps->succ_cnt ++;
    }

    long current_time = time(NULL);
    if (current_time - qps->last_time >= 1) {
        //如果当前时间比最后记录时间大于1秒，那么我们进行记录
        printf("---> qps = %d <----\n", qps->succ_cnt);
        qps->last_time = current_time;//记录最后时间
        qps->succ_cnt = 0;//清空成功次数
    }

    //给服务端发送新的请求
    request.set_id(response.id() + 1);
    request.set_content(response.content());

    std::string requestString;
    request.SerializeToString(&requestString);

    conn->send(msgid, requestString.c_str(), requestString.size());
}


//创建链接成功之后
void connection_start(net_connection::sptr client)
{
    qps_test::EchoMessage request;

    request.set_id(1);
    request.set_content("Hello Lars!!!");

    std::string requestString;

    request.SerializeToString(&requestString);

    int msgid = 1;//与server端的消息路由一致
    client->send(msgid, requestString.c_str(), requestString.size());
}


void *thread_main(void *args)
{
    (void)args;
    //给服务端发包
     
    event_loop loop; 

    tcp_client::sptr client = std::make_shared<tcp_client>(&loop, "127.0.0.1", 7777);

    Qps qps;

    //设置回调
    client->add_router(1, std::bind(&busi, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, &qps));

    //设置链接创建成功之后Hook
    client->set_on_build_callback(connection_start);
    client->do_connect();

    loop.loop();

    return NULL;
}


int main(int argc, char **argv) 
{
    if (argc == 1) {
        printf("Usage: ./client [threadNum]\n");
        return 1;
    }

    //创建N个线程
    int thread_num = atoi(argv[1]);
    pthread_t *tids;
    tids = new pthread_t[thread_num];

    for (int i = 0; i < thread_num; i++) {
        pthread_create(&tids[i], NULL, thread_main, NULL);
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}
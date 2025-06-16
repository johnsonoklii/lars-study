#include "lars_reactor.h"
#include "lars.pb.h"

void report_status(net_connection::sptr conn) {

    lars::ReportStatusRequest req; 

    //组装测试消息
    req.set_modid(rand() % 3 + 1);
    req.set_cmdid(1);
    req.set_caller(123);
    req.set_ts(time(NULL));

    for (int i = 0; i < 3; i ++) {
        lars::HostCallResult result;
        result.set_ip(i + 1);
        result.set_port((i + 1) * (i + 1));

        result.set_succ(100);
        result.set_err(3);
        result.set_overload(true);
        req.add_results()->CopyFrom(result);
    }


    std::string requestString;
    req.SerializeToString(&requestString);

    //发送给reporter service
    conn->send(lars::ID_ReportStatusRequest, requestString.c_str(), requestString.size());
}


void connection_build(net_connection::sptr conn)
{
    report_status(conn);
}


int main()
{
    event_loop loop;

    tcp_client::sptr client = std::make_shared<tcp_client>(&loop, "127.0.0.1", 7779);

    //添加建立连接成功业务
    client->set_on_build_callback(connection_build);
    client->do_connect();

    loop.loop();
    
    return 0;
}
#include "lars_reactor.h"
#include "store_report.h"
#include "store_thread_pool.h"
#include "lars.pb.h"

void get_report_status(net_connection::sptr conn, int msgid, const char* data, int len);

class reporter_server {
public:
    reporter_server() {
        m_loop = new event_loop();
        std::string ip = config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
        int port = config_file::instance()->GetNumber("reactor", "port", 7779);
        int thread_num = config_file::instance()->GetNumber("reactor", "threadNum", 3);

        server = new tcp_server(m_loop, ip.c_str(), port);
        server->set_thraed_num(thread_num);

        // 注册handler
        server->add_router(lars::MessageId::ID_ReportStatusRequest, get_report_status);
    }

    ~reporter_server() {
        delete m_loop;
        delete server;
    }

    void start() {
        // 任务线程池
        store_thread_pool::get_instance().start();

        server->start();
        printf("lars reporter service ....\n");
        m_loop->loop();
    }
private:
    tcp_server* server;
    event_loop* m_loop;
};

void get_report_status(net_connection::sptr conn, int msgid, const char* data, int len) {
    // 1. 解析
    lars::ReportStatusRequest req;
    req.ParseFromArray(data, len);

    (void)conn;
    (void)msgid;

    task_msg task;
    task.type = task_msg::TASK_TYPE::kNEW_TASK;
    task.task_cb = [req](event_loop*) {
        // 2. 将上报数据存储到db 
        printf("store report status\n");
        store_reporter sr;
        sr.store(req);
    };

    store_thread_pool::get_instance().add_task(task);
}

int main() {
    config_file::setPath("conf/lars_reporter.conf");
    reporter_server server;
    server.start();
    
    return 0;
}
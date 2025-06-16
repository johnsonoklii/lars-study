#include "store_report.h"
#include "config_file.h"

store_reporter::store_reporter() {
        //1 初始化
    //1.1 多线程使用mysql需要先调用mysql_library_init
    mysql_library_init(0, NULL, NULL);

    //1.2 初始化链接，和设置超时时间
    mysql_init(&m_db_conn);
    mysql_options(&m_db_conn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
    my_bool reconnect = 1;
    mysql_options(&m_db_conn, MYSQL_OPT_RECONNECT, &reconnect);


    //2 加载配置
    std::string db_host = config_file::instance()->GetString("mysql", "db_host", "127.0.0.1");
    short db_port = config_file::instance()->GetNumber("mysql", "db_port", 3306);
    std::string db_user = config_file::instance()->GetString("mysql", "db_user", "root");
    std::string db_passwd = config_file::instance()->GetString("mysql", "db_passwd", "Wsq5201314.");
    std::string db_name = config_file::instance()->GetString("mysql", "db_name", "lars_dns");


    //3 链接数据库
    if ( mysql_real_connect(&m_db_conn, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), db_port, NULL, 0) == NULL)  {
        fprintf(stderr, "mysql real connect error\n");
        exit(1);
    }
}

store_reporter::~store_reporter() {
    mysql_close(&m_db_conn);
}

void store_reporter::store(lars::ReportStatusRequest req) {
    for (int i = 0; i < req.results_size(); i++) {
        //一条report 调用记录
        const lars::HostCallResult &result = req.results(i);
        int overload = result.overload() ? 1: 0;
        char sql[1024];
        
        snprintf(sql, 1024, "INSERT INTO ServerCallStatus"
                "(modid, cmdid, ip, port, caller, succ_cnt, err_cnt, ts, overload) "
                "VALUES (%d, %d, %u, %u, %u, %u, %u, %u, %d) ON DUPLICATE KEY "
                "UPDATE succ_cnt = %u, err_cnt = %u, ts = %u, overload = %d",
                req.modid(), req.cmdid(), result.ip(), result.port(), req.caller(),
                result.succ(), result.err(), req.ts(), overload,
                result.succ(), result.err(), req.ts(), overload);

        mysql_ping(&m_db_conn);//ping 测试一下，防止链接断开，会触发重新建立连接

        if (mysql_real_query(&m_db_conn, sql, strlen(sql)) != 0) {
            fprintf(stderr, "Fial to Insert into ServerCallStatus %s\n", mysql_error(&m_db_conn));
        }
    }
}
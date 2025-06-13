#include "dns_route.h"
#include "config_file.h"
#include "subscribe.h"

#include <string>

#include <unistd.h>
#include <time.h>
#include <string.h>

extern tcp_server* server;

route::route() {
    m_data_pointer = std::make_shared<route_map>();
    m_tmp_pointer = std::make_shared<route_map>();

    connect_db();
    build_maps();
    load_version();

    m_check_thread = std::thread(&route::check_route_changes);
    m_check_thread.detach(); // FIXME: 修改
}

void route::connect_db() {
    // --- mysql数据库配置---
    std::string db_host = config_file::instance()->GetString("mysql", "db_host", "127.0.0.1");
    int db_port = config_file::instance()->GetNumber("mysql", "db_port", 3306);
    std::string db_user = config_file::instance()->GetString("mysql", "db_user", "root");
    std::string db_passwd = config_file::instance()->GetString("mysql", "db_passwd", "Wsq5201314.");
    std::string db_name = config_file::instance()->GetString("mysql", "db_name", "lars_dns");

    mysql_init(&m_db_conn);

    // 超时断开
    mysql_options(&m_db_conn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
    // 设置mysql链接断开后自动重连
    my_bool reconnect = 1; 
    mysql_options(&m_db_conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(&m_db_conn, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), db_port, NULL, 0)) {
        fprintf(stderr, "Failed to connect mysql\n");
        exit(1);
    }
}

void route::build_maps() {
    int ret = 0;

    snprintf(m_sql, 1000, "SELECT * FROM RouteData;");
    ret = mysql_real_query(&m_db_conn, m_sql, strlen(m_sql));
    if ( ret != 0) {
        fprintf(stderr, "failed to find any data, error %s\n", mysql_error(&m_db_conn));
        exit(1);
    }

    // 得到结果集
    MYSQL_RES *result = mysql_store_result(&m_db_conn);
    // 得到行数
    long line_num = mysql_num_rows(result);
    MYSQL_ROW row;
    for (long i = 0; i < line_num; i++) {
        row = mysql_fetch_row(result);
        int modID = atoi(row[1]);
        int cmdID = atoi(row[2]);
        unsigned ip = atoi(row[3]);
        int port = atoi(row[4]);

        //组装map的key，有modID/cmdID组合
        uint64_t key = ((uint64_t)modID << 32) + cmdID;
        uint64_t value = ((uint64_t)ip << 32) + port;

        printf("modID = %d, cmdID = %d, key = %ld, ip = %u, port = %d\n", modID, cmdID, key, ip, port);

        //插入到RouterDataMap_A中
        (*m_data_pointer)[key].insert(value);
    }

    mysql_free_result(result);
}


host_set route::get_hosts(int modid, int cmdid) {
    host_set hosts;     
    uint64_t key = ((uint64_t)modid << 32) + cmdid;
    
    // TODO: 1. copy and read，2.或者读写锁
    route_map_sptr tmp_data = get_data_pointer(); 
    auto it = tmp_data->find(key);
    if (it != tmp_data->end()) {
        hosts = it->second;
    }

    return hosts;
}

/*
*  return 0, 表示 加载成功，version没有改变
*         1, 表示 加载成功，version有改变
*         -1 表示 加载失败
* */
int route::load_version() {
    snprintf(m_sql, 1000, "SELECT version from RouteVersion WHERE id = 1;");

    int ret = mysql_real_query(&m_db_conn, m_sql, strlen(m_sql));
    if (ret) {
        fprintf(stderr, "load version error: %s\n", mysql_error(&m_db_conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(&m_db_conn);
    if (!result) {
        fprintf(stderr, "mysql store result: %s\n", mysql_error(&m_db_conn));
        mysql_free_result(result);
        return -1;
    }

    long line_num = mysql_num_rows(result);
    if (line_num == 0) {
        fprintf(stderr, "No version in table RouteVersion: %s\n", mysql_error(&m_db_conn));
        mysql_free_result(result);
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    long new_version = atol(row[0]);
    if (new_version == this->m_version) {
        //加载成功但是没有修改
        mysql_free_result(result);
        return 0;
    }

    this->m_version = new_version;
    printf("now route version is %ld\n", this->m_version);

    mysql_free_result(result);
    return 1;
}

int route::load_route_data() {
    // TODO: 1. copy and swap 2. 读写锁
    // route_map_sptr new_pointer = std::make_shared<route_map>();
    m_tmp_pointer->clear();
    snprintf(m_sql, 100, "SELECT * FROM RouteData;");

    int ret = mysql_real_query(&m_db_conn, m_sql, strlen(m_sql));
    if (ret) {
        fprintf(stderr, "load version error: %s\n", mysql_error(&m_db_conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(&m_db_conn);
    if (!result) {
        fprintf(stderr, "mysql store result: %s\n", mysql_error(&m_db_conn));
        mysql_free_result(result);
        return -1;
    }

    long line_num = mysql_num_rows(result);
    MYSQL_ROW row;
    for (long i = 0;i < line_num; ++i) {
        row = mysql_fetch_row(result);
        int modid = atoi(row[1]);
        int cmdid = atoi(row[2]);
        unsigned ip = atoi(row[3]);
        int port = atoi(row[4]);

        uint64_t key = ((uint64_t)modid << 32) + cmdid;
        uint64_t value = ((uint64_t)ip << 32) + port;

        (*m_tmp_pointer)[key].insert(value);
    }
    printf("load data to tmep succ! size is %lu\n", m_tmp_pointer->size());

    mysql_free_result(result);

    // std::lock_guard<std::mutex> lock(m_tmp_pointer_mutex);
    // if (!m_tmp_pointer.unique()) {
    //     route_map_sptr copy_data(new route_map(*m_tmp_pointer));
    //     m_tmp_pointer.swap(copy_data);
    // }
    // m_tmp_pointer.swap(new_pointer);

    return 0;
}

//加载RouteChange得到修改的modid/cmdid
void route::load_changes(std::vector<uint64_t> &change_list) {
    //读取当前版本之前的全部修改 
    // 为什么是 <=？不应该是=？
    // 因为在没有推送修改的mod/cmd时，如果又修改了，就会出现version <= m_version的情况
    // 比如1s查询一次数据库，在0s时，修改了，m_version=0。如果在0.5s时，又修改了，m_version=0.5。此时就需要将 <=0.5的都推送出去。
    snprintf(m_sql, 1000, "SELECT modid,cmdid FROM RouteChange WHERE version <= %ld;", m_version);

    int ret = mysql_real_query(&m_db_conn, m_sql, strlen(m_sql));
    if (ret) {
        fprintf(stderr, "mysql_real_query: %s\n", mysql_error(&m_db_conn));
        return ;
    }

    MYSQL_RES *result = mysql_store_result(&m_db_conn);
    if (!result) {
        fprintf(stderr, "mysql_store_result %s\n", mysql_error(&m_db_conn));
        return ;
    }

    long lineNum = mysql_num_rows(result);
    if (lineNum == 0) {
        fprintf(stderr,  "No version in table ChangeLog: %s\n", mysql_error(&m_db_conn));
        return ;
    }

    MYSQL_ROW row;
    for (long i = 0;i < lineNum; ++i) {
        row = mysql_fetch_row(result);
        int modid = atoi(row[0]);
        int cmdid = atoi(row[1]);
        uint64_t key = (((uint64_t)modid) << 32) + cmdid;
        change_list.push_back(key);
    }
    mysql_free_result(result);  
}

//将RouteChange
//删除RouteChange的全部修改记录数据,remove_all为全部删除
//否则默认删除当前版本之前的全部修改
void route::remove_changes(bool remove_all) {
    if (remove_all == false) {
        snprintf(m_sql, 1000, "DELETE FROM RouteChange WHERE version <= %ld;", m_version);
    } else {
        snprintf(m_sql, 1000, "DELETE FROM RouteChange;");
    }

    int ret = mysql_real_query(&m_db_conn, m_sql, strlen(m_sql));
    if (ret != 0) {
        fprintf(stderr, "delete RouteChange: %s\n", mysql_error(&m_db_conn));
        return ;
    } 

    return;
}

void route::swap() {
    // route_map_sptr new_pointer = get_tmp_pointer();
    // copy and write
    std::lock_guard<std::mutex> lock(m_data_pointer_mutex);
    if (!m_data_pointer.unique()) {
        // 如果有人在使用m_data_pointer，不能直接 m_data_pointer.swap(m_tmp_pointer);这样m_tmp_pointer会指向m_data_pointer的源对象数据
        // 如果此时修改了m_tmp_pointer的数据（没有加锁），m_data_pointer的源对象也会被修改，线程安全问题
        route_map_sptr copy_data(new route_map(*m_data_pointer));
        m_data_pointer.swap(copy_data);
    }
    
    // 没人使用，就放心修改，因为m_data_pointer加锁了
    m_data_pointer.swap(m_tmp_pointer);  
}


//周期性后端检查db的route信息的更新变化业务
void route::check_route_changes() {
    int wait_time = 10;//10s自动修改一次，也可以从配置文件读取
    long last_load_time = time(NULL);

    //清空全部的RouteChange
    route::get_instance().remove_changes(true);

     //1 判断是否有修改
    while (true) {
        ::sleep(1); // 每秒查询一次
        long current_time = time(NULL);

        //1.1 加载RouteVersion得到当前版本号
        int ret = route::get_instance().load_version();
        if (ret == 1) {
            //version改版 有modid/cmdid修改
            //2 如果有修改
            //2.1 将最新的RouteData加载到m_tmp_pointer中
            if (route::get_instance().load_route_data() == 0) {
                //2.2 更新m_tmp_pointer数据到m_data_pointer中
                route::get_instance().swap();
                last_load_time = current_time;//更新最后加载时间
            }

            //2.3 获取被修改的modid/cmdid对应的订阅客户端,进行推送         
            std::vector<uint64_t> changes;
            route::get_instance().load_changes(changes);

            //推送 
            // TODO: 这里考虑thread_local
            // subscribe_list::get_instance().publish(changes);

            // 发送一个publish任务，
            server->send_task(std::bind(&subscribe_list::publish, std::placeholders::_1, changes));

            //2.4 删除当前版本之前的修改记录
            route::get_instance().remove_changes();
        }
        else {
            //3 如果没有修改
            if (current_time - last_load_time >= wait_time) {
                //3.1 超时,加载最新的temp_pointer
                if (route::get_instance().load_route_data() == 0) {
                    //3.2 m_tmp_pointer数据更新到m_data_pointer中
                    route::get_instance().swap();
                    last_load_time = current_time;
                }
            }
        }
    }
}
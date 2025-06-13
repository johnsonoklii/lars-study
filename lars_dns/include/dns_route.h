#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <thread>

#include <stdint.h>

#include <mysql/mysql.h>

// modeid/cmdid -> iplist
using host_set = std::unordered_set<uint64_t>;
using route_map = std::unordered_map<uint64_t, host_set>;
using route_map_sptr = std::shared_ptr<route_map>;

class route {
public:
    route(const route&) = delete;   
    route& operator=(const route&) = delete;

    static route& get_instance() {
        static route instance;
        return instance;
    }

    void build_maps();

    host_set get_hosts(int modid, int cmdid);
    // 加载routeversion
    int load_version();
    // 加载数据到m_tmp_pointer
    int load_route_data();
    void load_changes(std::vector<uint64_t> &change_list); 
    void remove_changes(bool remove_all = false);

    //周期性后端检查db的route信息的更新变化业务
    static void check_route_changes();

    route_map_sptr get_tmp_pointer() {
        // std::lock_guard<std::mutex> lock(m_tmp_pointer_mutex);
        return m_tmp_pointer;
    }

    route_map_sptr get_data_pointer() {
        std::lock_guard<std::mutex> lock(m_data_pointer_mutex);
        return m_data_pointer;
    }

private:
    route();

    void connect_db();
    void swap();

private:
    MYSQL m_db_conn;
    char m_sql[1024];
    long m_version{0};

    std::mutex m_data_pointer_mutex;
    // std::mutex m_tmp_pointer_mutex; // m_tmp_pointer只会在后台更新线程中使用，不需要加锁

    route_map_sptr m_data_pointer;
    route_map_sptr m_tmp_pointer;

    std::thread m_check_thread;
};
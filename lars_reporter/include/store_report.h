#pragma once

#include "lars.pb.h"

#include <mysql/mysql.h>

class store_reporter {
public:
    store_reporter();   
    ~store_reporter();

    void store(lars::ReportStatusRequest req);

private:
    MYSQL m_db_conn;
};
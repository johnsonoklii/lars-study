#include "store_thread_pool.h"
#include "config_file.h"

store_thread_pool::store_thread_pool() {
    m_loop_pool = std::unique_ptr<event_loop_thread_pool>(new event_loop_thread_pool(nullptr));

    int thread_num = config_file::instance()->GetNumber("repoter", "db_thread_cnt", 3);
    m_loop_pool->set_thread_num(thread_num);
}

void store_thread_pool::start() {
    m_loop_pool->start();
    printf("store_thread_pool::start() success\n");
}

void store_thread_pool::add_task(const task_msg& task) {
    event_loop* loop = m_loop_pool->get_next_loop();
    if (loop == nullptr) {
        printf("store_thread_pool::add_task(): get_next_loop is nullptr\n");
        return;
    }

    loop->add_task(task);
}
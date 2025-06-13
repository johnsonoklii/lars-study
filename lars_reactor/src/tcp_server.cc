#include "tcp_server.h"
#include "reactor_buf.h"
#include "buf_pool.h"
#include "tcp_conn.h"
#include "config_file.h"
#include "task_msg.h"

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

constexpr int MAX_CONNS = 1024;

std::atomic<int> tcp_server::s_conn_nums{0};

tcp_server::tcp_server(event_loop* loop, const char* ip, uint16_t port)
: m_listen_sock(csocket::create_socket(AF_INET)) {

    m_max_conn_nums = config_file::instance()->GetNumber("reactor", "maxConn", MAX_CONNS);

    //忽略一些信号 SIGHUP, SIGPIPE
    //SIGPIPE:如果客户端关闭，服务端再次write就会产生
    //SIGHUP:如果terminal关闭，会给当前进程发送该信号
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal ignore SIGHUP\n");
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal ignore SIGPIPE\n");
    }

    // 1. 线程池
    m_thread_pool = std::unique_ptr<event_loop_thread_pool>(new event_loop_thread_pool(loop));
    
    // 2. 初始化地址
    struct sockaddr_in server_addr;
    bzero (&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // 2.1 设置SO_REUSEADDR
    m_listen_sock.set_reuse_addr(true);
    m_listen_sock.set_reuse_port(true);

    // 2.2 绑定地址
    m_listen_sock.bind(server_addr);

    // 2.3 监听
    m_listen_sock.listen();
    m_loop = loop;
    m_loop->add_io_event(m_listen_sock.fd(), EPOLLIN, std::bind(&tcp_server::accept_callback, this));
}

tcp_server::~tcp_server() {

}

void tcp_server::start() {
    m_thread_pool->start();
}

void tcp_server::accept_callback() {
    while(true) {
        int connfd;  
        printf("begin accept\n");
        struct sockaddr_in conn_addr;
        connfd = m_listen_sock.accept(&conn_addr);
        if (connfd < 0) {
            if (errno == EINTR) {
                fprintf(stderr, "accept errno=EINTR\n");
                continue;
            }
            else if (errno == EMFILE) {
                //建立链接过多，资源不够
                fprintf(stderr, "accept errno=EMFILE\n");
            }
            else if (errno == EAGAIN) {
                fprintf(stderr, "accept errno=EAGAIN\n");
                break;
            }
            else {
                fprintf(stderr, "accept error");
                exit(1);
            }
        } else {
            if (s_conn_nums.load() > m_max_conn_nums) {
                fprintf(stderr, "so many connections, max = %d\n", m_max_conn_nums);
                close(connfd);
                break;
            }

            event_loop* loop = m_thread_pool->get_next_loop();
            tcp_conn::sptr conn = std::make_shared<tcp_conn>(loop, connfd);
            if (conn == nullptr) {
                fprintf(stderr, "new tcp_conn error");
                exit(1);
            }
            conn->set_close_callback(std::bind(&tcp_server::delete_conn, this, connfd));
            conn->set_on_build_callback(m_on_build_callback);

            task_msg task;
            task.type = task_msg::TASK_TYPE::kNEW_TASK;
            task.task_cb = std::bind(&tcp_conn::listen_read, conn);
            loop->add_task(std::move(task));
        
            m_conns[connfd] = conn;
            break;
        }
    }
}

void tcp_server::delete_conn(int fd) {
    task_msg task;
    task.type = task_msg::TASK_TYPE::kNEW_TASK;
    task.task_cb = std::bind(&tcp_server::delete_conn_inloop, this, fd);
    m_loop->add_task(std::move(task));
}

void tcp_server::delete_conn_inloop(int fd) {
    auto conn = m_conns[fd];
    if (m_on_close_callback) {
        m_on_close_callback(conn);
    }
    m_conns.erase(fd);
}

// tcp_conn::sptr tcp_server::get_conn(int fd) {
//     // TODO: 线程安全，loop应该有一个thread_local的conns，这样可以不从全局拿
//     auto it = m_conns.find(fd);
//     if (it == m_conns.end()) {
//         return nullptr;
//     }
//     return it->second;
// }

void tcp_server::send_task(task_callback_t cb) {
    task_msg task;
    task.type = task_msg::TASK_TYPE::kNEW_TASK;
    task.task_cb = cb;
    m_loop->add_task(task);

    std::vector<event_loop*> all_loop = m_thread_pool->get_all_loop();
    for (auto loop : all_loop) {
        loop->add_task(task);
    }
}
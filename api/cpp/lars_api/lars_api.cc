#include "lars_api.h"
#include "lars.pb.h"

#include <arpa/inet.h>

lars_client::lars_client() {
    printf("lars_client()\n");
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 本机，api和agent server部署在同一台机器

    m_socks.resize(3);
    for (int i = 0; i < 3; ++i) {
        int sockfd = csocket::create_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        m_socks[i].set_fd(sockfd);
        server_addr.sin_port = htons(8888 + i);
        
        int ret = connect(m_socks[i].fd(), (const struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret == -1) {
            perror("connect()");
            exit(1);
        }
        printf("connection agent udp server succ!\n");
    }
}

int lars_client::get_host(int modid, int cmdid, std::string& ip, int& port) {
    uint32_t seq = m_seqid++;  
    // 1. 发送请求
    lars::GetHostRequest req;
    req.set_modid(modid);
    req.set_cmdid(cmdid);
    req.set_seq(seq);

    std::string req_str;
    req.SerializeToString(&req_str);

    message header;
    header.id = lars::ID_GetHostRequest;
    header.len = MESSAGE_HEADER_SIZE + req_str.size();

    char write_buf[header.len];
    memcpy(write_buf, &header, MESSAGE_HEADER_SIZE);
    memcpy(write_buf + MESSAGE_HEADER_SIZE, req_str.c_str(), req_str.size());

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    int index = (modid + cmdid) % 3; // 随机选一个agent server
    int ret = ::sendto(m_socks[index].fd(), write_buf, header.len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("sendto()..\n");
        return lars::RET_SYSTEM_ERROR;
    }

    // 2. 接收响应
    char read_buf[2048];
    lars::GetHostResponse resp;
    do {
        int msg_len = ::recvfrom(m_socks[index].fd(), read_buf, sizeof(read_buf), 0, NULL, NULL);
        if (msg_len < 0) {
            perror("recvfrom()..\n");
            return lars::RET_SYSTEM_ERROR;
        }
        if (msg_len == 0) {
            printf("server close the connection\n");
            return lars::RET_SYSTEM_ERROR;
        }

        bzero(&header, sizeof(header));
        memcpy(&header, read_buf, MESSAGE_HEADER_SIZE);

        char body[msg_len - MESSAGE_HEADER_SIZE];
        memcpy(body, read_buf + MESSAGE_HEADER_SIZE, msg_len - MESSAGE_HEADER_SIZE);

        int ret = resp.ParseFromArray(body, msg_len - MESSAGE_HEADER_SIZE);
        if (ret < 0) {
            fprintf(stderr, "message format error: head.msglen = %d, message_len = %d, message_len - MESSAGE_HEAD_LEN = %d, head msgid = %d, ID_GetHostResponse = %d\n", header.len, msg_len, msg_len-MESSAGE_HEADER_SIZE, header.id, lars::ID_GetHostResponse);
            return lars::RET_SYSTEM_ERROR;
        }
    } while (resp.seq() < seq); // 如果连续发送了两个请求seq=0, seq=1，只处理最新的即seq=1

    // 3. 逻辑处理
    if (resp.seq() != seq || resp.modid() != modid || resp.cmdid() != cmdid) {
        fprintf(stderr, "message format error\n");
        return lars::RET_SYSTEM_ERROR;
    }

    if (resp.retcode() == 0) {
        lars::HostInfo host = resp.host();

        struct in_addr inaddr;
        inaddr.s_addr = host.ip();
        ip = inet_ntoa(inaddr);
        port = host.port();
    }

    return resp.retcode(); //lars::RET_SUCC
}
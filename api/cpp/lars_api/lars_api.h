#pragma once 

#include "lars_reactor.h"

#include <vector>

class lars_client {
public:
    lars_client();

    int get_host(int modid, int cmdid, std::string& ip, int& port);

private:
    std::vector<csocket> m_socks;
    uint32_t m_seqid{0}; //消息的序列号
};
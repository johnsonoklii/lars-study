#pragma once

struct message {
    int id;
    int len; // header + body
};

constexpr int MESSAGE_HEADER_SIZE = sizeof(message);
constexpr int MESSAGE_MAX_SIZE = 65535000 - MESSAGE_HEADER_SIZE; // 数据包体最大长度
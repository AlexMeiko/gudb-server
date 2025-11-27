#pragma once
#include "../core/Buffer.h"
#include "../protocol/Parser.h"
#include "../db/Database.h"

namespace gudb::net {
    class Connection {
    public:
        Connection(int fd, Database *db);

        ~Connection();

        /**
         * @brief 处理读事件（ET模式）
         * 循环读取所有可用数据，直到返回 EAGAIN 或 EWOULDBLOCK
         */
        void handleRead();

        /**
         * @brief 处理写事件（ET模式）
         * 处理可写事件，发送写缓冲区中的数据
         */
        void handleWrite();

        int fd() const { return fd_; }

    private:
        int fd_;
        core::Buffer readBuf_;
        core::Buffer writeBuf_;
        protocol::Parser parser_;
        Database *db_;

        // 处理接收到的命令，从读缓冲区解析并执行命令
        void processCommands();

        // 发送回复给客户端
        void sendReply(const std::string &reply);
    };
} // namespace gudb::net

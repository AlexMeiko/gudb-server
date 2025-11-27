#pragma once
#include "../db/Database.h"
#include "Connection.h"
#include <unordered_map>
#include <memory>

namespace gudb::net {
    class Server {
    public:
        Server(Database *db);

        ~Server();

        /**
         * @brief 启动监听
         * @param ip 监听IP地址
         * @param port 监听端口
         * @return 成功返回 true，失败返回 false
         * 创建 socket，绑定地址，开始监听，并添加到 epoll
         */
        bool listen(const std::string &ip, int port);

        /**
         * @brief 主事件循环
         * 使用 epoll 处理 I/O 事件，包括新连接、读事件、写事件和错误事件
         */
        void run();

    private:
        int epollFd_;
        int listenFd_;
        Database *db_;
        std::unordered_map<int, std::unique_ptr<Connection> > connections_;

        // 接受新连接，循环接受所有待处理的新连接，创建 Connection 对象并添加到 epoll
        void acceptConnection();

        // 处理读事件，调用对应 Connection 的 handleRead 方法
        void handleRead(int fd);

        // 处理写事件，调用对应 Connection 的 handleWrite 方法
        void handleWrite(int fd);

        // 移除连接，从连接映射中删除，从 epoll 中移除，关闭文件描述符
        void removeConnection(int fd);
    };
} // namespace gudb::net
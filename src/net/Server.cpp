#include "Server.h"
#include "../core/Logger.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace gudb::net {
    Server::Server(Database *db) : epollFd_(-1), listenFd_(-1), db_(db) {
        epollFd_ = epoll_create1(0);
        if (epollFd_ < 0) {
            LOG_ERROR("Failed to create epoll");
            exit(1);
        }
    }

    Server::~Server() {
        close(epollFd_);
        close(listenFd_);
    }

    // 启动监听
    // 创建 socket，绑定地址，开始监听，并添加到 epoll
    bool Server::listen(const std::string &ip, int port) {
        listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd_ < 0) {
            LOG_ERROR("Failed to create socket");
            return false;
        }

        // 设置 socket 为非阻塞模式
        int flags = fcntl(listenFd_, F_GETFL, 0);
        if (fcntl(listenFd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            LOG_ERROR("Failed to set non-blocking mode");
            close(listenFd_);
            return false;
        }

        int opt = 1;
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (bind(listenFd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            LOG_ERROR("Failed to bind to " + ip + ":" + std::to_string(port));
            return false;
        }

        if (::listen(listenFd_, 511) < 0) {
            LOG_ERROR("Failed to listen");
            return false;
        }

        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = listenFd_;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev);

        LOG_INFO("Server listening on " + ip + ":" + std::to_string(port));
        return true;
    }

    // 主事件循环
    // 使用 epoll 处理 I/O 事件，包括新连接、读事件、写事件和错误事件
    void Server::run() {
        constexpr int MAX_EVENTS = 64;
        epoll_event events[MAX_EVENTS];

        while (true) {
            int n = epoll_wait(epollFd_, events, MAX_EVENTS, -1);
            if (n < 0) {
                if (errno == EINTR) continue;
                LOG_ERROR("epoll_wait error");
                break;
            }

            for (int i = 0; i < n; ++i) {
                int fd = events[i].data.fd;

                if (fd == listenFd_) {
                    acceptConnection();
                } else {
                    if (events[i].events & EPOLLIN) {
                        handleRead(fd);
                    }
                    if (events[i].events & EPOLLOUT) {
                        handleWrite(fd);
                    }
                    if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                        removeConnection(fd);
                    }
                }
            }
        }
    }

    // 接受新连接
    // 循环接受所有待处理的新连接，创建 Connection 对象并添加到 epoll
    void Server::acceptConnection() {
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t len = sizeof(clientAddr);
            int clientFd = accept4(listenFd_, reinterpret_cast<sockaddr *>(&clientAddr), &len, SOCK_NONBLOCK);

            if (clientFd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                LOG_ERROR("accept error");
                continue;
            }

            epoll_event ev{};

            // 监听读事件（ET），EPOLLRDHUP 用于检测对端半关闭（避免漏处理断连）
            ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
            ev.data.fd = clientFd;
            epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev);

            connections_[clientFd] = std::make_unique<Connection>(clientFd, db_);
            LOG_INFO("New connection from " + std::string(inet_ntoa(clientAddr.sin_addr)) +
                ":" + std::to_string(ntohs(clientAddr.sin_port)));
        }
    }

    // 处理读事件
    // 调用对应 Connection 的 handleRead 方法
    void Server::handleRead(int fd) {
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            it->second->handleRead();
        }
    }

    // 处理写事件
    // 调用对应 Connection 的 handleWrite 方法
    void Server::handleWrite(int fd) {
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            it->second->handleWrite();
        }
    }

    // 移除连接
    // 从连接映射中删除，从 epoll 中移除，关闭文件描述符
    void Server::removeConnection(int fd) {
        connections_.erase(fd);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        LOG_INFO("Connection closed: fd=" + std::to_string(fd));
    }
} // namespace gudb::net

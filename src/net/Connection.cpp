#include "Connection.h"
#include <cerrno>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include "../cmd/Registry.h"
#include "../protocol/Encoder.h"

namespace gudb::net {
    Connection::Connection(int fd, Database *db, int epollFd) : fd_(fd), epollFd_(epollFd), db_(db) {}

    Connection::~Connection() { close(fd_); }

    // 处理读事件（ET）
    // 循环读取所有可用数据，直到返回 EAGAIN 或 EWOULDBLOCK
    bool Connection::handleRead() {
        while (true) {
            int err = 0;
            ssize_t n = readBuf_.readFd(fd_, &err);

            if (n > 0) {
                continue;
            }

            if (n == 0) {
                // 客户端断开连接
                return false;
            }

            if (err == EINTR) {
                continue;
            }

            if (err == EAGAIN || err == EWOULDBLOCK) {
                break; // 读完数据
            }

            return false;
        }

        return processCommands();
    }

    // 处理接收到的命令
    // 从读缓冲区解析并执行命令
    bool Connection::processCommands() {
        while (true) {
            std::vector<std::string> args;
            auto result = parser_.parse(readBuf_, args);

            if (result == gudb::protocol::ParseResult::WAIT) {
                break; // 数据不够，等待更多数据
            }

            if (result == gudb::protocol::ParseResult::ERROR) {
                // 协议错误
                sendReply(gudb::protocol::Encoder::encodeError("protocol error"));
                // 丢弃错误数据以重新同步解析状态
                readBuf_.retrieve(readBuf_.readableBytes());
                break;
            }

            if (args.empty()) {
                continue; // 空命令
            }

            // 从注册表中查找并执行命令
            auto cmdFunc = gudb::cmd::Registry::instance().getCommand(args[0]);
            std::string reply;

            if (cmdFunc) {
                reply = cmdFunc(args, *db_);
            } else {
                reply = gudb::protocol::Encoder::encodeError("unknown command '" + args[0] + "'");
            }

            // 发送回复给客户端
            sendReply(reply);
        }

        if (writeBuf_.readableBytes() > 0) {
            return handleWrite();
        }
        return true;
    }

    // 发送回复给客户端
    void Connection::sendReply(const std::string &reply) { writeBuf_.append(reply.c_str(), reply.size()); }

    // 处理写事件（ET 模式）
    // 在 EPOLLOUT 触发时，将写缓冲区写出
    bool Connection::handleWrite() {
        while (writeBuf_.readableBytes() > 0) {
            ssize_t n = write(fd_, writeBuf_.peek(), writeBuf_.readableBytes());
            if (n > 0) {
                writeBuf_.retrieve(static_cast<size_t>(n));
                continue;
            }

            if (n == 0) {
                return false;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (!listeningEpollOut_) {
                    epoll_event ev{};
                    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT;
                    ev.data.fd = fd_;
                    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &ev);
                    listeningEpollOut_ = true;
                }
                return true;
            }

            return false;
        }

        if (listeningEpollOut_) {
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
            ev.data.fd = fd_;
            epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd_, &ev);
            listeningEpollOut_ = false;
        }
        return true;
    }
} // namespace gudb::net

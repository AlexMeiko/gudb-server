#include "Connection.h"
#include "../cmd/Registry.h"
#include "../protocol/Encoder.h"
#include <unistd.h>
#include <cerrno>
#include <memory>

namespace gudb::net {
    Connection::Connection(int fd, Database *db) : fd_(fd), db_(db) {}

    Connection::~Connection() {
        close(fd_);
    }

    // 处理读事件（ET）
    // 循环读取所有可用数据，直到返回 EAGAIN 或 EWOULDBLOCK
    void Connection::handleRead() {
        while (true) {
            char buf[4096];
            ssize_t n = read(fd_, buf, sizeof(buf));

            if (n > 0) {
                // 读取到数据
                readBuf_.append(buf, n);
            } else if (n == 0) {
                // 客户端断开连接
                return;
            } else {
                //读取错误
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break; // 读完数据
                }
                return;
            }
        }

        processCommands();
    }

    // 处理接收到的命令
    // 从读缓冲区解析并执行命令
    void Connection::processCommands() {
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
    }

    // 发送回复给客户端
    // TODO: 使用写缓冲区处理 EAGAIN 情况
    void Connection::sendReply(const std::string &reply) {
        write(fd_, reply.c_str(), reply.size());
    }

    // 处理写事件（ET 模式）
    // TODO: 完善写事件处理，使用写缓冲区
    void Connection::handleWrite() {}
} // namespace gudb::net
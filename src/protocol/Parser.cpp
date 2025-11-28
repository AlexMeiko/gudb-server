#include "Parser.h"
#include <string>

namespace gudb::protocol {
    ParseResult Parser::parse(core::Buffer &buffer, std::vector<std::string> &outArgs) {
        if (buffer.readableBytes() == 0) {
            return ParseResult::WAIT;
        }

        char first = *buffer.peek();
        // 当前仅支持 RESP 数组请求，其他前缀视为协议错误

        if (first == '*') {
            return parseArray(buffer, outArgs);
        }

        return ParseResult::ERROR;
    }

    ParseResult Parser::parseArray(core::Buffer &buffer, std::vector<std::string> &outArgs) {
        std::string line = buffer.retrieveUntilCrlf();
        if (line.empty()) {
            // "*<count>\r\n" 尚未收全，继续等待
            return ParseResult::WAIT;
        }

        if (line[0] != '*') {
            return ParseResult::ERROR;
        }

        int count = std::stoi(line.substr(1));
        if (count <= 0) {
            return ParseResult::ERROR;
        }

        outArgs.reserve(count);

        for (int i = 0; i < count; ++i) {
            std::string arg;
            ParseResult res = parseBulkString(buffer, arg);
            if (res != ParseResult::OK) {
                return res;
            }
            outArgs.push_back(arg);
        }

        return ParseResult::OK;
    }

    ParseResult Parser::parseBulkString(core::Buffer &buffer, std::string &out) {
        std::string line = buffer.retrieveUntilCrlf();
        if (line.empty()) {
            return ParseResult::WAIT;
        }

        if (line[0] != '$') {
            return ParseResult::ERROR;
        }

        int len = std::stoi(line.substr(1));
        if (len < 0) {
            // RESP 协议中的空值，返回空字符串占位
            out = "";
            return ParseResult::OK;
        }

        if (buffer.readableBytes() < static_cast<size_t>(len) + 2) {
            // 数据区或末尾 CRLF 未到齐，保持等待
            return ParseResult::WAIT;
        }

        out.assign(buffer.peek(), len);
        buffer.retrieve(len + 2);

        return ParseResult::OK;
    }
} // namespace gudb::protocol
#include "Parser.h"
#include <algorithm>
#include <string>
#include <vector>
#include <charconv>

namespace gudb::protocol {
    // BufferReader 辅助类，用于不修改原 Buffer 的情况下进行预读取
    class Parser::BufferReader {
    public:
        explicit BufferReader(core::Buffer &buffer) : buffer_(buffer), offset_(0) {}

        size_t readableBytes() const { return buffer_.readableBytes() - offset_; }

        const char *peek() const { return buffer_.peek() + offset_; }

        void retrieve(size_t len) { offset_ += len; }

        std::string retrieveUntilCrlf() {
            const char *start = peek();
            const char *end = buffer_.peek() + buffer_.readableBytes();
            const char *crlf = std::search(start, end, "\r\n", std::next("\r\n", 2));

            if (crlf == end) {
                return "";
            }

            std::string res(start, crlf);
            retrieve(res.size() + 2);
            return res;
        }

        size_t bytesConsumed() const { return offset_; }

    private:
        core::Buffer &buffer_;
        size_t offset_;
    };

    ParseResult Parser::parse(core::Buffer &buffer,
                              std::vector<std::string> &outArgs) {
        if (buffer.readableBytes() == 0) {
            return ParseResult::WAIT;
        }

        BufferReader reader(buffer);
        char first = *reader.peek();

        if (first == '*') {
            ParseResult res = parseArray(reader, outArgs);
            if (res == ParseResult::OK) {
                // 解析成功，消耗 Buffer 数据
                buffer.retrieve(reader.bytesConsumed());
            }
            return res;
        }

        return ParseResult::ERROR;
    }

    ParseResult Parser::parseArray(BufferReader &reader,
                                   std::vector<std::string> &outArgs) {
        std::string line = reader.retrieveUntilCrlf();
        if (line.empty()) {
            return ParseResult::WAIT;
        }

        if (line[0] != '*') {
            return ParseResult::ERROR;
        }

        int count = 0;
        auto [ptr, ec] = std::from_chars(line.data() + 1, line.data() + line.size(), count);
        if (ec != std::errc() || ptr != line.data() + line.size()) {
            return ParseResult::ERROR;
        }

        if (count <= 0) {
            return ParseResult::ERROR;
        }

        outArgs.reserve(count);

        for (int i = 0; i < count; ++i) {
            std::string arg;
            ParseResult res = parseBulkString(reader, arg);
            if (res != ParseResult::OK) {
                return res;
            }
            outArgs.push_back(arg);
        }

        return ParseResult::OK;
    }

    ParseResult Parser::parseBulkString(BufferReader &reader, std::string &out) {
        std::string line = reader.retrieveUntilCrlf();
        if (line.empty()) {
            return ParseResult::WAIT;
        }

        if (line[0] != '$') {
            return ParseResult::ERROR;
        }

        int len = 0;
        auto [ptr, ec] = std::from_chars(line.data() + 1, line.data() + line.size(), len);
        if (ec != std::errc() || ptr != line.data() + line.size()) {
            return ParseResult::ERROR;
        }

        if (len < 0) {
            out = "";
            return ParseResult::OK;
        }

        if (reader.readableBytes() < static_cast<size_t>(len) + 2) {
            return ParseResult::WAIT;
        }

        out.assign(reader.peek(), len);
        reader.retrieve(len + 2);

        return ParseResult::OK;
    }
} // namespace gudb::protocol
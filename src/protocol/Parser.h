#pragma once
#include "../core/Buffer.h"
#include <vector>
#include <string>

namespace gudb::protocol {
    enum class ParseResult { OK, WAIT, ERROR };

    class Parser {
    public:
        ParseResult parse(core::Buffer &buffer, std::vector<std::string> &outArgs);

    private:
        class BufferReader;

        ParseResult parseArray(BufferReader &reader, std::vector<std::string> &outArgs);

        ParseResult parseBulkString(BufferReader &reader, std::string &out);
    };
} // namespace gudb::protocol

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
        ParseResult parseArray(core::Buffer &buffer, std::vector<std::string> &outArgs);

        ParseResult parseBulkString(core::Buffer &buffer, std::string &out);
    };
} // namespace gudb::protocol

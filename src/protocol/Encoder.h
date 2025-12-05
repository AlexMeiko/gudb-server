#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace gudb::protocol {
    class Encoder {
    public:
        static std::string encodeNull();

        static std::string encodeError(const std::string &msg);

        static std::string encodeSimpleString(const std::string &str);

        static std::string encodeBulkString(const std::string &str);

        static std::string encodeInteger(int64_t value);

        static std::string encodeArray(const std::vector<std::string> &arr);
    };
} // namespace gudb::protocol
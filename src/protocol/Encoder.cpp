#include "Encoder.h"

namespace gudb::protocol {
    std::string Encoder::encodeNull() {
        return "$-1\r\n";
    }

    std::string Encoder::encodeError(const std::string &msg) {
        return "-" + msg + "\r\n";
    }

    std::string Encoder::encodeSimpleString(const std::string &str) {
        return "+" + str + "\r\n";
    }

    std::string Encoder::encodeBulkString(const std::string &str) {
        if (str.empty()) {
            return "$0\r\n\r\n";
        }
        return "$" + std::to_string(str.size()) + "\r\n" + str + "\r\n";
    }

    std::string Encoder::encodeInteger(int64_t value) {
        return ":" + std::to_string(value) + "\r\n";
    }

    std::string Encoder::encodeArray(const std::vector<std::string> &arr) {
        std::string result = "*" + std::to_string(arr.size()) + "\r\n";
        for (const auto &elem: arr)
            result += encodeBulkString(elem);
        return result;
    }
} // namespace gudb::protocol

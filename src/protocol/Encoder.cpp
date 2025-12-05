#include "Encoder.h"
#include <string>
#include <cstdint>
#include <vector>

#if __cpp_lib_format >= 202110L
#include <format>
#define USE_STD_FORMAT
#else
#include <sstream>
#endif

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
#ifdef USE_STD_FORMAT
        return std::format("${}\r\n{}\r\n", str.size(), str);
#else
        std::ostringstream oss;
        oss << "$" << str.size() << "\r\n" << str << "\r\n";
        return oss.str();
#endif
    }

    std::string Encoder::encodeInteger(int64_t value) {
#ifdef USE_STD_FORMAT
        return std::format(":{}\r\n", value);
#else
        std::ostringstream oss;
        oss << ":" << value << "\r\n";
        return oss.str();
#endif
    }

    std::string Encoder::encodeArray(const std::vector<std::string> &arr) {
#ifdef USE_STD_FORMAT
        std::string result = std::format("*{}\r\n", arr.size());
#else
        std::ostringstream oss;
        oss << "*" << arr.size() << "\r\n";
        std::string result = oss.str();
#endif
        for (const auto &elem: arr)
            result += encodeBulkString(elem);
        return result;
    }
} // namespace gudb::protocol
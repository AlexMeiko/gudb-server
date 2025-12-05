#pragma once
#include <string>
#include <cstdint>
#include <charconv>
#include <limits>
#include "../protocol/Encoder.h"

namespace gudb::cmd {
    inline std::string doIncrLike(std::string &val, int64_t delta) {
        int64_t current = 0;

        if (val.empty()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), current);

        if (ec != std::errc{} || ptr != val.data() + val.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        if ((delta > 0 && current > std::numeric_limits<int64_t>::max() - delta) ||
            (delta < 0 && current < std::numeric_limits<int64_t>::min() - delta)) {
            return protocol::Encoder::encodeError("ERR increment or decrement would overflow");
        }

        val = std::to_string(current + delta);

        return protocol::Encoder::encodeInteger(current + delta);
    }
} // namespace gudb::cmd
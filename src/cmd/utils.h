#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#if __cpp_lib_format >= 202110L
#include <format>
#define USE_STD_FORMAT
#else
#include <iomanip>
#include <locale>
#include <sstream>
#endif

#include "../ds/ZSkipList.h"
#include "../protocol/Encoder.h"

namespace gudb::cmd {
    inline std::string formatDouble(double value) {
#ifdef USE_STD_FORMAT
        return std::format("{:.{}g}", value, std::numeric_limits<double>::max_digits10);
#else
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
        return oss.str();
#endif
    }

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

    inline std::string parseScoreBound(const std::string &token, double &score, bool &inclusive) {
        if (token.empty()) {
            return protocol::Encoder::encodeError("ERR value is not a valid float");
        }

        std::string_view view(token);
        inclusive = true;

        // 判开闭区间
        if (view.front() == '(') {
            inclusive = false;
            view.remove_prefix(1);
            if (view.empty()) {
                return protocol::Encoder::encodeError("ERR value is not a valid float");
            }
        } else if (view.front() == '[') {
            return protocol::Encoder::encodeError("ERR value is not a valid float");
        }

        /// 忽略大小写判等
        auto strIcaseEqual = [](std::string_view a, std::string_view b) {
            if (a.size() != b.size()) {
                return false;
            }
            for (size_t i = 0; i < a.size(); ++i) {
                char ca = a[i];
                char cb = b[i];
                if (ca >= 'A' && ca <= 'Z') {
                    ca = static_cast<char>(ca - 'A' + 'a');
                }
                if (cb >= 'A' && cb <= 'Z') {
                    cb = static_cast<char>(cb - 'A' + 'a');
                }
                if (ca != cb) {
                    return false;
                }
            }
            return true;
        };

        // 处理正负无穷
        if (strIcaseEqual(view, "-inf") || strIcaseEqual(view, "-infinity")) {
            score = -std::numeric_limits<double>::infinity();
            return {};
        }

        if (strIcaseEqual(view, "+inf") || strIcaseEqual(view, "inf") || strIcaseEqual(view, "+infinity") ||
            strIcaseEqual(view, "infinity")) {
            score = std::numeric_limits<double>::infinity();
            return {};
        }

        // 解析浮点数
        auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), score);
        if (ec != std::errc{} || ptr != view.data() + view.size() || std::isnan(score)) {
            return protocol::Encoder::encodeError("ERR value is not a valid float");
        }

        return {};
    }

    inline std::string parseLexBound(const std::string &token, ZSkipList::LexBound &out) {

        // 处理正负无穷
        if (token == "-" || token == "+") {
            out.type = (token == "-") ? ZSkipList::LexBound::Type::NEG_INF : ZSkipList::LexBound::Type::POS_INF;
            out.inclusive = true;
            out.value.clear();
            return {};
        }

        // 检查合法性
        if (token.empty()) {
            return protocol::Encoder::encodeError("ERR min or max is not a valid string range item");
        }

        char prefix = token.front();
        if (prefix != '[' && prefix != '(') {
            return protocol::Encoder::encodeError("ERR min or max is not a valid string range item");
        }

        out.type = ZSkipList::LexBound::Type::VALUE;
        out.inclusive = prefix == '[';
        out.value = token.substr(1);
        return {};
    }
} // namespace gudb::cmd

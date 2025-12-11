#include <algorithm>
#include <charconv>
#include <utility>
#include "../protocol/Encoder.h"
#include "Registry.h"

namespace gudb::cmd {
    std::string zaddCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() < 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zadd' command");
        }

        // 额外多出的 score 或 member 会触发语法错误，避免部分写入
        if (((args.size() - 2) & 1U) != 0) {
            return protocol::Encoder::encodeError("ERR syntax error");
        }

        const std::string &key = args[1];

        std::vector<std::pair<double, std::string>> parsed;
        parsed.reserve((args.size() - 2) / 2);
        for (size_t i = 2; i < args.size(); i += 2) {
            const std::string &scoreStr = args[i];
            const std::string &member = args[i + 1];

            double score = 0.0;
            auto [ptr, ec] = std::from_chars(scoreStr.data(), scoreStr.data() + scoreStr.size(), score);
            if (ec != std::errc{} || ptr != scoreStr.data() + scoreStr.size()) {
                return protocol::Encoder::encodeError("ERR value is not a valid float");
            }

            parsed.emplace_back(score, member);
        }

        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GZSet{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &zset = std::get<GZSet>(obj->value);
        int added = 0;
        for (const auto &[score, member]: parsed) {
            if (zset.insertOrUpdate(score, member)) {
                added++;
            }
        }

        return protocol::Encoder::encodeInteger(added);
    }

    std::string zrangeCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zrange' command");
        }

        const std::string &key = args[1];
        const std::string &startStr = args[2];
        const std::string &stopStr = args[3];

        long long start = 0, stop = 0;
        auto [ptrStart, ecStart] = std::from_chars(startStr.data(), startStr.data() + startStr.size(), start);
        auto [ptrStop, ecStop] = std::from_chars(stopStr.data(), stopStr.data() + stopStr.size(), stop);
        if (ecStart != std::errc{} || ptrStart != startStr.data() + startStr.size() || ecStop != std::errc{} ||
            ptrStop != stopStr.data() + stopStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeArray({});
        }
        if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &zset = std::get<GZSet>(obj->value);
        int sz = zset.size();
        if (sz == 0) {
            return protocol::Encoder::encodeArray({});
        }

        long long l = start < 0 ? start + sz : start;
        long long r = stop < 0 ? stop + sz : stop;
        l = std::max<long long>(l, 0);
        r = std::min<long long>(r, sz - 1);

        if (l > r) {
            return protocol::Encoder::encodeArray({});
        }

        std::vector<std::string> result;
        zset.getRange(static_cast<int>(l), static_cast<int>(r), result);
        return protocol::Encoder::encodeArray(result);
    }
} // namespace gudb::cmd

static gudb::cmd::AutoRegister reg_zadd("ZADD", gudb::cmd::zaddCommand);
static gudb::cmd::AutoRegister reg_zrange("ZRANGE", gudb::cmd::zrangeCommand);

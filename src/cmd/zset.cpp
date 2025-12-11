#include <algorithm>
#include <charconv>
#include <utility>
#include "../protocol/Encoder.h"
#include "Registry.h"

namespace gudb::cmd {
    // ZADD 添加/更新有序集合成员
    std::string zaddCommand(std::vector<std::string> &args, Database &db) {
        // 参数校验：至少包含 key 和一对 score/member，且必须成对
        if (args.size() < 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zadd' command");
        }

        if (((args.size() - 2) & 1U) != 0) { // 多出的 score 或 member 触发语法错误，避免部分写入
            return protocol::Encoder::encodeError("ERR syntax error");
        }

        const std::string &key = args[1];

        // 解析 score/member
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

        // 获取或创建目标 ZSET
        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GZSet{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        // 插入或更新成员，统计新增数量
        auto &zset = std::get<GZSet>(obj->value);
        int added = 0;
        for (const auto &[score, member]: parsed) {
            if (zset.insertOrUpdate(score, member)) {
                added++;
            }
        }

        return protocol::Encoder::encodeInteger(added);
    }

    // ZRANGE 按索引范围返回成员
    std::string zrangeCommand(std::vector<std::string> &args, Database &db) {
        // 参数校验：必须为 key start stop
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zrange' command");
        }

        const std::string &key = args[1];
        const std::string &startStr = args[2];
        const std::string &stopStr = args[3];

        // 解析范围索引（支持负数）
        long long start = 0, stop = 0;
        auto [ptrStart, ecStart] = std::from_chars(startStr.data(), startStr.data() + startStr.size(), start);
        auto [ptrStop, ecStop] = std::from_chars(stopStr.data(), stopStr.data() + stopStr.size(), stop);
        if (ecStart != std::errc{} || ptrStart != startStr.data() + startStr.size() || ecStop != std::errc{} ||
            ptrStop != stopStr.data() + stopStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        // 获取并校验 ZSET
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

        // 归一化索引到合法范围
        long long l = start < 0 ? start + sz : start;
        long long r = stop < 0 ? stop + sz : stop;
        l = std::max<long long>(l, 0);
        r = std::min<long long>(r, sz - 1);

        if (l > r) {
            return protocol::Encoder::encodeArray({});
        }

        // 生成返回结果
        std::vector<std::string> result;
        zset.getRange(static_cast<int>(l), static_cast<int>(r), result);
        return protocol::Encoder::encodeArray(result);
    }

    // ZRANGEBYSCORE 按分数范围返回成员
    std::string zrangebyscoreCommand(std::vector<std::string> &args, Database &db) {
        // 参数：key min max
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zrangebyscore' command");
        }

        const std::string &key = args[1];
        const std::string &minScoreStr = args[2];
        const std::string &maxScoreStr = args[3];

        // 解析分数范围
        double minScore = 0.0, maxScore = 0.0;
        auto [ptrMin, ecMin] = std::from_chars(minScoreStr.data(), minScoreStr.data() + minScoreStr.size(), minScore);
        auto [ptrMax, ecMax] = std::from_chars(maxScoreStr.data(), maxScoreStr.data() + maxScoreStr.size(), maxScore);
        if (ecMin != std::errc{} || ptrMin != minScoreStr.data() + minScoreStr.size() || ecMax != std::errc{} ||
            ptrMax != maxScoreStr.data() + maxScoreStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not a valid float");
        }

        // 获取并校验 ZSET
        Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeArray({});
        }
        if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        auto &zset = std::get<GZSet>(obj->value);

        // 获取区间内成员（闭区间）
        std::vector<std::string> result;
        zset.getRangeByScore(minScore, maxScore, result);
        return protocol::Encoder::encodeArray(result);
    }

    static gudb::cmd::AutoRegister reg_zadd("ZADD", gudb::cmd::zaddCommand);
    static gudb::cmd::AutoRegister reg_zrange("ZRANGE", gudb::cmd::zrangeCommand);
    static gudb::cmd::AutoRegister reg_zrangebyscore("ZRANGEBYSCORE", gudb::cmd::zrangebyscoreCommand);

} // namespace gudb::cmd

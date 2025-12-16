#include <algorithm>
#include <cctype>
#include <charconv>
#include <utility>
#include "../protocol/Encoder.h"
#include "Registry.h"
#include "utils.h"

namespace gudb::cmd {
    // ZADD 添加/更新有序集合成员
    std::string zaddCommand(const std::vector<std::string> &args, Database &db) {
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
            if (ec != std::errc{} || ptr != scoreStr.data() + scoreStr.size() || std::isnan(score)) {
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

    // ZCARD 获取有序集合成员数量
    std::string zcardCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zcard' command");
        }

        const std::string &key = args[1];

        Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeInteger(0);
        }
        if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &zset = std::get<GZSet>(obj->value);
        return protocol::Encoder::encodeInteger(zset.size());
    }

    // ZCOUNT 统计分数区间内成员数量
    std::string zcountCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zcount' command");
        }

        const std::string &key = args[1];
        const std::string &minScoreStr = args[2];
        const std::string &maxScoreStr = args[3];

        double minScore = 0.0, maxScore = 0.0;
        bool minInclusive = true, maxInclusive = true;
        if (auto err = parseScoreBound(minScoreStr, minScore, minInclusive); !err.empty()) {
            return err;
        }
        if (auto err = parseScoreBound(maxScoreStr, maxScore, maxInclusive); !err.empty()) {
            return err;
        }

        Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeInteger(0);
        }
        if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &zset = std::get<GZSet>(obj->value);
        int cnt = zset.countByScore(minScore, minInclusive, maxScore, maxInclusive);
        return protocol::Encoder::encodeInteger(cnt);
    }

    // ZINCRBY 为有序集合 member 的 score 增加 increment
    std::string zincrbyCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zincrby' command");
        }

        const std::string &key = args[1];
        const std::string &incrementStr = args[2];
        const std::string &member = args[3];

        double increment = 0.0;
        auto [ptr, ec] = std::from_chars(incrementStr.data(), incrementStr.data() + incrementStr.size(), increment);
        if (ec != std::errc{} || ptr != incrementStr.data() + incrementStr.size() || std::isnan(increment)) {
            return protocol::Encoder::encodeError("ERR value is not a valid float");
        }

        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GZSet{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &zset = std::get<GZSet>(obj->value);
        double newScore = zset.incrBy(member, increment);
        return protocol::Encoder::encodeBulkString(formatDouble(newScore));
    }

    // ZLEXCOUNT 统计字典序区间内成员数量
    std::string zlexcountCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zlexcount' command");
        }

        const std::string &key = args[1];
        const std::string &minStr = args[2];
        const std::string &maxStr = args[3];

        Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeInteger(0);
        }
        if (obj->type != ObjType::ZSET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        ZSkipList::LexBound minBound, maxBound;
        if (auto err = parseLexBound(minStr, minBound); !err.empty()) {
            return err;
        }
        if (auto err = parseLexBound(maxStr, maxBound); !err.empty()) {
            return err;
        }

        auto &zset = std::get<GZSet>(obj->value);
        const int cnt = zset.countByLex(minBound, maxBound);
        return protocol::Encoder::encodeInteger(cnt);
    }

    // ZRANGE 按索引范围返回成员
    std::string zrangeCommand(const std::vector<std::string> &args, Database &db) {
        // 参数：key start stop [WITHSCORES]
        if (args.size() != 4 && args.size() != 5) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zrange' command");
        }

        const std::string &key = args[1];
        const std::string &startStr = args[2];
        const std::string &stopStr = args[3];

        bool withScores = false;
        if (args.size() == 5) {
            std::string opt = args[4];
            std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);
            if (opt != "WITHSCORES") {
                return protocol::Encoder::encodeError("ERR syntax error");
            }
            withScores = true;
        }

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
        if (!withScores) {
            std::vector<std::string> result;
            zset.getRange(static_cast<int>(l), static_cast<int>(r), result);
            return protocol::Encoder::encodeArray(result);
        }

        std::vector<std::pair<double, std::string>> range;
        zset.getRange(static_cast<int>(l), static_cast<int>(r), range);

        std::vector<std::string> result;
        result.reserve(range.size() << 1);
        for (const auto &[score, member]: range) {
            result.push_back(member);
            result.push_back(formatDouble(score));
        }
        return protocol::Encoder::encodeArray(result);
    }

    // ZRANGEBYSCORE 按分数范围返回成员
    std::string zrangebyscoreCommand(const std::vector<std::string> &args, Database &db) {
        // 参数：key min max [WITHSCORES]
        if (args.size() != 4 && args.size() != 5) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zrangebyscore' command");
        }

        const std::string &key = args[1];
        const std::string &minScoreStr = args[2];
        const std::string &maxScoreStr = args[3];

        bool withScores = false;
        if (args.size() == 5) {
            std::string opt = args[4];
            std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);
            if (opt != "WITHSCORES") {
                return protocol::Encoder::encodeError("ERR syntax error");
            }
            withScores = true;
        }

        // 解析分数范围
        double minScore = 0.0, maxScore = 0.0;
        bool minInclusive = true;
        bool maxInclusive = true;
        if (auto err = parseScoreBound(minScoreStr, minScore, minInclusive); !err.empty()) {
            return err;
        }
        if (auto err = parseScoreBound(maxScoreStr, maxScore, maxInclusive); !err.empty()) {
            return err;
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
        if (!withScores) {
            std::vector<std::string> result;
            zset.getRangeByScore(minScore, minInclusive, maxScore, maxInclusive, result);
            return protocol::Encoder::encodeArray(result);
        }

        std::vector<std::pair<double, std::string>> range;
        zset.getRangeByScore(minScore, minInclusive, maxScore, maxInclusive, range);

        std::vector<std::string> result;
        result.reserve(range.size() << 1);
        for (const auto &[score, member]: range) {
            result.push_back(member);
            result.push_back(formatDouble(score));
        }
        return protocol::Encoder::encodeArray(result);
    }

    std::string zinterstoreCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'zinterstore' command");
        }

        const std::string &destination = args[1];
        const std::string &numkeysStr = args[2];

        long long numkeys = 0;
        auto [ptr, ec] = std::from_chars(numkeysStr.data(), numkeysStr.data() + numkeysStr.size(), numkeys);
        if (ec != std::errc{} || ptr != numkeysStr.data() + numkeysStr.size() || numkeys <= 0) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        const size_t expectedSize = static_cast<size_t>(numkeys) + 3;
        if (args.size() != expectedSize) {
            return protocol::Encoder::encodeError("ERR syntax error");
        }

        std::vector<GZSet *> zsets;
        zsets.reserve(static_cast<size_t>(numkeys));

        size_t baseIdx = 0;
        int minSize = std::numeric_limits<int>::max();
        for (size_t i = 3; i < args.size(); ++i) {
            Object *obj = db.get(args[i]);
            if (!obj) {
                minSize = 0;
                continue;
            }
            if (obj->type != ObjType::ZSET) {
                return protocol::Encoder::encodeError(
                        "WRONGTYPE Operation against a key holding the wrong kind of value");
            }

            auto &zset = std::get<GZSet>(obj->value);
            const int sz = zset.size();
            zsets.push_back(&zset);
            if (sz < minSize) {
                minSize = sz;
                baseIdx = zsets.size() - 1;
            }
        }

        if (minSize == 0) {
            db.remove(destination);
            return protocol::Encoder::encodeInteger(0);
        }

        std::vector<std::string> members;
        members.reserve(static_cast<size_t>(minSize));
        zsets[baseIdx]->getRange(0, minSize - 1, members);

        GZSet result;
        for (const auto &member: members) {
            double totalScore = 0.0;
            bool exists = true;

            for (const GZSet *zset: zsets) {
                double score = 0.0;
                if (!zset->getScore(member, score)) {
                    exists = false;
                    break;
                }
                totalScore += score;
            }

            if (exists) {
                result.insertOrUpdate(totalScore, member);
            }
        }

        const int siz = result.size();
        if (siz) {
            db.set(destination, Object(std::move(result)));
        } else {
            db.remove(destination);
        }
        return protocol::Encoder::encodeInteger(siz);
    }

    static gudb::cmd::AutoRegister reg_zadd("ZADD", gudb::cmd::zaddCommand);
    static gudb::cmd::AutoRegister reg_zcard("ZCARD", gudb::cmd::zcardCommand);
    static gudb::cmd::AutoRegister reg_zcount("ZCOUNT", gudb::cmd::zcountCommand);
    static gudb::cmd::AutoRegister reg_zincrby("ZINCRBY", gudb::cmd::zincrbyCommand);
    static gudb::cmd::AutoRegister reg_zlexcount("ZLEXCOUNT", gudb::cmd::zlexcountCommand);
    static gudb::cmd::AutoRegister reg_zrange("ZRANGE", gudb::cmd::zrangeCommand);
    static gudb::cmd::AutoRegister reg_zrangebyscore("ZRANGEBYSCORE", gudb::cmd::zrangebyscoreCommand);
    static gudb::cmd::AutoRegister reg_zinterstore("ZINTERSTORE", gudb::cmd::zinterstoreCommand);

} // namespace gudb::cmd

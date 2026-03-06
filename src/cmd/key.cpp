#include <algorithm>
#include <charconv>
#include <chrono>
#include <limits>
#include <string>
#include <vector>
#include "../protocol/Encoder.h"
#include "Registry.h"

namespace gudb::cmd {
    // DEL 删除一个或多个 key
    std::string delCommand(const std::vector<std::string> &args, Database &db) {
        // 参数校验：至少 1 个 key
        if (args.size() < 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'del' command");
        }

        return protocol::Encoder::encodeInteger(
                std::count_if(args.begin() + 1, args.end(), [&db](const auto &key) { return db.remove(key); }));
    }

    // EXISTS 统计存在的 key 数量
    std::string existsCommand(const std::vector<std::string> &args, Database &db) {
        // 参数校验：至少 1 个 key
        if (args.size() < 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'exists' command");
        }

        return protocol::Encoder::encodeInteger(
                std::count_if(args.begin() + 1, args.end(), [&db](const auto &key) { return db.exists(key); }));
    }

    // PEXPIRE 设置毫秒级过期
    std::string pexpireCommand(const std::vector<std::string> &args, Database &db) {
        // 参数校验：key milliseconds
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'pexpire' command");
        }

        const std::string &key = args[1];

        // 检查 key 是否存在
        if (!db.exists(key)) {
            return protocol::Encoder::encodeInteger(0);
        }

        const std::string &deltaStr = args[2];
        long long delta = 0;
        auto [ptr, ec] = std::from_chars(deltaStr.data(), deltaStr.data() + deltaStr.size(), delta);
        if (ec != std::errc() || ptr != deltaStr.data() + deltaStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        if (delta <= 0) {
            db.remove(key);
            return protocol::Encoder::encodeInteger(1);
        }

        // 获取当前时间戳（毫秒）
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

        long long expiresAt = now;
        if (delta > std::numeric_limits<long long>::max() - now) {
            expiresAt = std::numeric_limits<long long>::max();
        } else {
            expiresAt = now + delta;
        }
        db.setExpire(key, expiresAt);

        return protocol::Encoder::encodeInteger(1);
    }

    // PEXPIREAT 设置毫秒级绝对过期时间
    std::string pexpireatCommand(const std::vector<std::string> &args, Database &db) {
        // 参数校验：key timestamp-ms
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'pexpireat' command");
        }

        const std::string &key = args[1];

        // 检查 key 是否存在
        if (!db.exists(key)) {
            return protocol::Encoder::encodeInteger(0);
        }

        const std::string &timestampStr = args[2];
        long long timestamp = 0;
        auto [ptr, ec] = std::from_chars(timestampStr.data(), timestampStr.data() + timestampStr.size(), timestamp);
        if (ec != std::errc() || ptr != timestampStr.data() + timestampStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

        if (timestamp <= now) {
            db.remove(key);
            return protocol::Encoder::encodeInteger(1);
        }

        db.setExpire(key, timestamp);

        return protocol::Encoder::encodeInteger(1);
    }


    // EXPIRE 设置秒级过期
    std::string expireCommand(std::vector<std::string> &args, Database &db) {
        // 参数校验：key seconds
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'expire' command");
        }

        args[2].append("000");
        return pexpireCommand(args, db);
    }

    // EXPIREAT 设置秒级绝对过期时间
    std::string expireatCommand(std::vector<std::string> &args, Database &db) {
        // 参数校验：key timestamp-s
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'expireat' command");
        }

        args[2].append("000");
        return pexpireatCommand(args, db);
    }

    // PERSIST 移除过期时间
    std::string persistCommand(const std::vector<std::string> &args, Database &db) {
        // 参数校验：必须为 key
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'persist' command");
        }

        const std::string &key = args[1];

        if (!db.exists(key)) {
            return protocol::Encoder::encodeInteger(0);
        }

        if (db.getExpire(key) == -1) {
            return protocol::Encoder::encodeInteger(0);
        }

        db.persist(key);
        return protocol::Encoder::encodeInteger(1);
    }

    // PTTL	以毫秒为单位返回 key 的剩余的过期时间
    std::string pttlCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'pttl' command");
        }

        const std::string &key = args[1];

        if (!db.exists(key)) {
            return protocol::Encoder::encodeInteger(-2);
        }

        return protocol::Encoder::encodeInteger(db.ttl(key));
    }

    // TTL	以秒为单位，返回给定 key 的剩余生存时间
    std::string ttlCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'ttl' command");
        }

        const std::string &key = args[1];

        if (!db.exists(key)) {
            return protocol::Encoder::encodeInteger(-2);
        }

        long long ttlMs = db.ttl(key);

        // 特殊返回值
        if (ttlMs <= -1) {
            return protocol::Encoder::encodeInteger(ttlMs);
        }

        return protocol::Encoder::encodeInteger(ttlMs / 1000);
    }

    // RANDOMKEY	从当前数据库中随机返回一个 key
    // std::string randomkeyCommand(const std::vector<std::string> &args, Database &db) {
    //
    // }

    // RENAME	修改 key 的名称
    std::string renameCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'rename' command");
        }

        const std::string &oldKey = args[1];
        const std::string &newKey = args[2];

        if (!db.exists(oldKey)) {
            return protocol::Encoder::encodeError("ERR no such key");
        }

        db.rename(oldKey, newKey);
        return protocol::Encoder::encodeSimpleString("OK");
    }

    // RENAMENX	当且仅当 newkey 不存在时，将 key 改名为 newkey
    std::string renameNXCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'renamenx' command");
        }
        const std::string &oldKey = args[1];
        const std::string &newKey = args[2];

        if (!db.exists(oldKey)) {
            return protocol::Encoder::encodeError("ERR no such key");
        }

        if (oldKey == newKey) {
            return protocol::Encoder::encodeInteger(0);
        }

        return protocol::Encoder::encodeInteger(db.rename(oldKey, newKey, false));
    }

    // TYPE

    std::string typeCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'type' command");
        }
        const std::string &key = args[1];
        Object *value = db.get(key);

        if (!value) {
            return protocol::Encoder::encodeSimpleString("none");
        }

        switch (value->type) {
            case ObjType::STRING:
                return protocol::Encoder::encodeSimpleString("string");
            case ObjType::LIST:
                return protocol::Encoder::encodeSimpleString("list");
            case ObjType::HASH:
                return protocol::Encoder::encodeSimpleString("hash");
            case ObjType::SET:
                return protocol::Encoder::encodeSimpleString("set");
            case ObjType::ZSET:
                return protocol::Encoder::encodeSimpleString("zset");
            default:
                return protocol::Encoder::encodeSimpleString("unknown");
        }
    }

    // 自注册
    static AutoRegister reg_del("DEL", delCommand);
    static AutoRegister reg_exists("EXISTS", existsCommand);
    static AutoRegister reg_expire("EXPIRE", expireCommand);
    static AutoRegister reg_expireat("EXPIREAT", expireatCommand);
    static AutoRegister reg_pexpire("PEXPIRE", pexpireCommand);
    static AutoRegister reg_pexpireat("PEXPIREAT", pexpireatCommand);
    static AutoRegister reg_persist("PERSIST", persistCommand);
    static AutoRegister reg_pttl("PTTL", pttlCommand);
    static AutoRegister reg_ttl("TTL", ttlCommand);
    static AutoRegister reg_rename("RENAME", renameCommand);
    static AutoRegister reg_renamenx("RENAMENX", renameNXCommand);
    static AutoRegister reg_type("TYPE", typeCommand);
} // namespace gudb::cmd

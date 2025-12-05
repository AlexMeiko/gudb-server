#include "Registry.h"
#include "utils.h"
#include "../protocol/Encoder.h"
#include <utility>
#include <charconv>
#include <vector>
#include <string>
#include <cstdint>

namespace gudb::cmd {
    std::string hsetCommand(std::vector<std::string> &args, Database &db) {
        //TODO: 改为支持同时插入多个字段值对
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'hset' command");
        }

        const std::string &key = args[1];
        const std::string &field = args[2];
        std::string value = args[3];

        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GHash{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::HASH) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &hash = std::get<GHash>(obj->value);
        bool isNewField = hash.find(field) == hash.end();
        hash[field] = std::move(value);

        return protocol::Encoder::encodeInteger(isNewField ? 1 : 0);
    }

    std::string hgetCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'hget' command");
        }

        const std::string &key = args[1];
        const std::string &field = args[2];

        // 检查键是否存在
        if (!db.exists(key)) {
            return protocol::Encoder::encodeNull(); // NULL
        }

        const auto &slot = db.get(key);

        if (slot->type != ObjType::HASH) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        const auto &hash = std::get<GHash>(slot->value);
        auto it = hash.find(field);

        if (it == hash.end()) {
            return protocol::Encoder::encodeNull(); // NULL
        }

        // 空字符串处理
        return protocol::Encoder::encodeBulkString(it->second);
    }

    // HINCRBY key field increment - 为哈希表 key 中的字段 field 的值加上增量 increment
    std::string hincrbyCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'hincrby' command");
        }

        const std::string &key = args[1];
        const std::string &field = args[2];

        // 解析增量
        int64_t delta;
        auto [ptr, ec] = std::from_chars(args[3].data(), args[3].data() + args[3].size(), delta);
        if (ec != std::errc{} || ptr != args[3].data() + args[3].size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        Object *obj = db.get(key);

        // 如果key 不存在，则创建
        if (!obj) {
            db.set(key, Object(GHash{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::HASH) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &hash = std::get<GHash>(obj->value);

        // 若字段不存在，初始化为"0"
        if (hash.find(field) == hash.end()) {
            hash[field] = "0";
        }

        return doIncrLike(hash[field], delta);
    }

    // 自注册
    static AutoRegister reg_hset("HSET", hsetCommand);
    static AutoRegister reg_hget("HGET", hgetCommand);
    static AutoRegister reg_hincrby("HINCRBY", hincrbyCommand);
} // namespace gudb::cmd
#include "Registry.h"
#include "../protocol/Encoder.h"

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

    // 自注册
    static AutoRegister reg_hset("HSET", hsetCommand);
    static AutoRegister reg_hget("HGET", hgetCommand);
} // namespace gudb::cmd

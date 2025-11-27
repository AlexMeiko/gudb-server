#include "Registry.h"
#include "../protocol/Encoder.h"

namespace gudb::cmd {
    std::string setCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() < 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'set' command");
        }

        const std::string &key = args[1];

        // 检查类型是否匹配
        Object *obj = db.get(key);
        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        db.set(key, Object(std::move(args[2])));

        return protocol::Encoder::encodeSimpleString("OK");
    }

    std::string getCommand(const std::vector<std::string> &args, Database &db) {
        // 1. 参数检查
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'get' command");
        }

        const std::string &key = args[1];

        // 2. 检查键是否存在
        const Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeNull();
        }

        // 3. 检查类型
        if (obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        // 4. 返回值
        const auto &value = std::get<GString>(obj->value);
        return protocol::Encoder::encodeBulkString(value);
    }

    // 自注册
    static AutoRegister reg_set("SET", setCommand);
    static AutoRegister reg_get("GET", getCommand);
} // namespace gudb::cmd

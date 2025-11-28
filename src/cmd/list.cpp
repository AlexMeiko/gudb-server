#include "Registry.h"
#include "../protocol/Encoder.h"
#include <utility>

namespace gudb::cmd {
    std::string lpushCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() < 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'lpush' command");
        }

        const std::string &key = args[1];

        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GList{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::LIST) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &list = std::get<GList>(obj->value);
        for (size_t i = 2; i < args.size(); ++i) {
            list.push_front(args[i]);
        }

        return protocol::Encoder::encodeInteger(list.size());
    }

    std::string lpopCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'lpop' command");
        }

        const std::string &key = args[1];

        // 检查键是否存在
        if (!db.exists(key)) {
            return protocol::Encoder::encodeNull();
        }

        const auto &slot = db.get(key);

        if (slot->type != ObjType::LIST) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &list = std::get<GList>(db.get(key)->value);

        if (list.empty()) {
            return protocol::Encoder::encodeNull();
        }

        std::string value = std::move(list.front());
        list.pop_front();

        return protocol::Encoder::encodeBulkString(value);
    }

    // 自注册
    static AutoRegister reg_lpush("LPUSH", lpushCommand);
    static AutoRegister reg_lpop("LPOP", lpopCommand);
} // namespace gudb::cmd
#include "Registry.h"
#include "../protocol/Encoder.h"
#include <utility>
#include <vector>
#include <string>

namespace gudb::cmd {
    std::string saddCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() < 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'sadd' command");
        }

        const std::string &key = args[1];

        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(GSet{}));
            obj = db.get(key);
        } else if (obj->type != ObjType::SET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        auto &set = std::get<GSet>(obj->value);
        int added = 0;
        for (size_t i = 2; i < args.size(); ++i) {
            if (set.insert(args[i]).second) {
                added++;
            }
        }

        return protocol::Encoder::encodeInteger(added);
    }

    std::string smembersCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'smembers' command");
        }

        const std::string &key = args[1];

        // 检查键是否存在
        if (!db.exists(key)) {
            return protocol::Encoder::encodeArray({});
        }

        const auto &slot = db.get(key);

        if (slot->type != ObjType::SET) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        const auto &set = std::get<GSet>(slot->value);

        std::vector<std::string> members(set.begin(), set.end());
        return protocol::Encoder::encodeArray(members);
    }

    // 自注册
    static AutoRegister reg_sadd("SADD", saddCommand);
    static AutoRegister reg_smembers("SMEMBERS", smembersCommand);
} // namespace gudb::cmd
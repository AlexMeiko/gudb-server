#include "Registry.h"
#include <numeric>
#include "../protocol/Encoder.h"

namespace gudb::cmd {
    // DEL 用于删除 key
    std::string delCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'del' command");
        }

        return protocol::Encoder::encodeInteger(std::count_if(args.begin() + 1, args.end(),
                                                              [&db](const auto &key) { return db.remove(key); })
        );
    }

    //EXISTS 检查给定 key 是否存在
    std::string existsCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'exists' command");
        }

        return protocol::Encoder::encodeInteger(std::count_if(args.begin() + 1, args.end(),
                                                              [&db](const auto &key) { return db.exists(key); })
        );
    }


    // EXPIRE	为给定 key 设置过期时间
    // EXPIREAT	用于为 key 设置过期时间，接受的时间参数是 UNIX 时间戳
    // PEXPIRE	设置 key 的过期时间，以毫秒计
    // PEXPIREAT	设置 key 过期时间的时间戳(unix timestamp)，以毫秒计
    // KEYS	查找所有符合给定模式的 key
    // MOVE	将当前数据库的 key 移动到给定的数据库中
    // PERSIST	移除 key 的过期时间，key 将持久保持
    // PTTL	以毫秒为单位返回 key 的剩余的过期时间
    // TTL	以秒为单位，返回给定 key 的剩余生存时间(
    // RANDOMKEY	从当前数据库中随机返回一个 key
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
    static AutoRegister reg_rename("RENAME", renameCommand);
    static AutoRegister reg_renamenx("RENAMENX", renameNXCommand);
    static AutoRegister reg_type("TYPE", typeCommand);
} // namespace gudb::cmd

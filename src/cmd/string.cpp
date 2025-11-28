#include "Registry.h"
#include "../protocol/Encoder.h"
#include <utility>
#include <algorithm>

namespace gudb::cmd {
    //SET   设置指定 key 的值
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

    // GET	获取指定 key 的值
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

    // GETRANGE	返回 key 中字符串值的子字符
    std::string getrangeCommand(const std::vector<std::string> &args, Database &db) {
        // 1. 参数检查
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'getrange' command");
        }

        const std::string &key = args[1], &startStr = args[2], &endStr = args[3];

        // 2. 检查键是否存在
        const Object *obj = db.get(key);
        if (!obj) {
            return protocol::Encoder::encodeBulkString("");
        }

        // 3. 检查类型
        if (obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        try {
            int l = std::stoi(startStr), r = std::stoi(endStr);

            // 获取字符串值
            const auto &value = std::get<GString>(obj->value);
            int len = static_cast<int>(value.length());

            // 处理索引
            l = std::max(l < 0 ? l + len : l, 0);
            r = std::min(r < 0 ? r + len : r, len - 1);

            if (l > r || l >= len) {
                return protocol::Encoder::encodeBulkString("");
            }

            return protocol::Encoder::encodeBulkString(value.substr(l, r - l + 1));
        } catch (const std::exception &e) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }
    }

    // GETSET	将给定 key 的值设为 value ，并返回 key 的旧值 ( old value )
    std::string getsetCommand(std::vector<std::string> &args, Database &db) {
        //1. 参数检查
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'getset' command");
        }

        std::string &key = args[1], &newValue = args[2];


        // 2. 检查键是否存在
        Object *obj = db.get(key);
        if (!obj) {
            db.set(key, Object(newValue));
            return protocol::Encoder::encodeNull();
        }

        // 3. 检查类型
        if (obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        std::string res = std::get<GString>(obj->value);
        std::get<GString>(obj->value).swap(newValue); //O(1) 交换值
        // std::get<GString>(obj->value) = std::move(args[2]); // O(n) 复制值,安全
        obj->expiresAt = -1;

        return protocol::Encoder::encodeBulkString(res);
    }

    // GETBIT	对 key 所储存的字符串值，获取指定偏移量上的位 ( bit )
    // MGET	获取所有(一个或多个)给定 key 的值
    // SETBIT	对 key 所储存的字符串值，设置或清除指定偏移量上的位(bit)
    // SETEX	设置 key 的值为 value 同时将过期时间设为 seconds
    // SETNX	只有在 key 不存在时设置 key 的值
    // SETRANGE	从偏移量 offset 开始用 value 覆写给定 key 所储存的字符串值
    // STRLEN	返回 key 所储存的字符串值的长度
    // MSET	同时设置一个或多个 key-value 对
    // MSETNX	同时设置一个或多个 key-value 对
    // PSETEX	以毫秒为单位设置 key 的生存时间
    // INCR	将 key 中储存的数字值增一
    // INCRBY	将 key 所储存的值加上给定的增量值 ( increment )
    // INCRBYFLOAT	将 key 所储存的值加上给定的浮点增量值 ( increment )
    // DECR	将 key 中储存的数字值减一
    // DECRBY	将 key 所储存的值减去给定的减量值 ( decrement )
    // APPEND	将 value 追加到 key 原来的值的末尾

    // 自注册
    static AutoRegister reg_set("SET", setCommand);
    static AutoRegister reg_get("GET", getCommand);
    static AutoRegister reg_getrange("GETRANGE", getrangeCommand);
    static AutoRegister reg_getset("GETSET", getsetCommand);
} // namespace gudb::cmd

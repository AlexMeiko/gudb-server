#include "Registry.h"
#include "utils.h"
#include "../protocol/Encoder.h"
#include <utility>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>
#include <string>
#include <cstdint>
#include <charconv>

namespace gudb::cmd {
    //SET   设置指定 key 的值
    // SET key value [NX|XX] [EX seconds|PX milliseconds|EXAT timestamp|PXAT timestamp-milliseconds]
    std::string setCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() < 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'set' command");
        }

        const std::string &key = args[1];
        std::string value = args[2];
        bool nx = false;
        bool xx = false;
        long long expireTime = -1;

        for (size_t i = 3; i < args.size(); ++i) {
            std::string opt = args[i];
            std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);

            if (opt == "NX") {
                nx = true;
            } else if (opt == "XX") {
                xx = true;
            } else if (opt == "EX" || opt == "PX") {
                if (expireTime != -1) {
                    return protocol::Encoder::encodeError("ERR syntax error");
                }

                if (i + 1 >= args.size()) {
                    return protocol::Encoder::encodeError("ERR syntax error");
                }

                const std::string &valStr = args[++i];
                long long val = 0;
                auto [ptr, ec] = std::from_chars(valStr.data(), valStr.data() + valStr.size(), val);
                if (ec != std::errc() || ptr != valStr.data() + valStr.size()) {
                    return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
                }

                if (val <= 0) {
                    return protocol::Encoder::encodeError("ERR invalid expire time in 'set' command");
                }

                if (opt == "EX") val *= 1000;

                auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();

                expireTime = now + val;
            } else if (opt == "EXAT" || opt == "PXAT") {
                if (expireTime != -1) {
                    return protocol::Encoder::encodeError("ERR syntax error");
                }

                if (i + 1 >= args.size()) {
                    return protocol::Encoder::encodeError("ERR syntax error");
                }

                const std::string &valStr = args[++i];
                long long val = 0;
                auto [ptr, ec] = std::from_chars(valStr.data(), valStr.data() + valStr.size(), val);
                if (ec != std::errc() || ptr != valStr.data() + valStr.size()) {
                    return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
                }

                if (val <= 0) {
                    return protocol::Encoder::encodeError("ERR invalid expire time in 'set' command");
                }
                if (opt == "EXAT") val *= 1000;
                expireTime = val;
            } else {
                return protocol::Encoder::encodeError("ERR syntax error");
            }
        }

        if (nx && xx) {
            return protocol::Encoder::encodeError("ERR syntax error");
        }

        const bool exists = db.exists(key);

        if (nx && exists) {
            return protocol::Encoder::encodeNull();
        }
        if (xx && !exists) {
            return protocol::Encoder::encodeNull();
        }

        db.set(key, Object(std::move(value), expireTime));

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

        int l, r;
        auto [ptr_l, ec_l] = std::from_chars(startStr.data(), startStr.data() + startStr.size(), l);
        auto [ptr_r, ec_r] = std::from_chars(endStr.data(), endStr.data() + endStr.size(), r);

        if (ec_l != std::errc{} || ptr_l != startStr.data() + startStr.size() ||
            ec_r != std::errc{} || ptr_r != endStr.data() + endStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        // 获取字符串值
        const auto &value = std::get<GString>(obj->value);
        int len = static_cast<int>(value.size());

        // 处理索引
        l = std::max(l < 0 ? l + len : l, 0);
        r = std::min(r < 0 ? r + len : r, len - 1);

        if (l > r || l >= len) {
            return protocol::Encoder::encodeBulkString("");
        }

        return protocol::Encoder::encodeBulkString(value.substr(l, r - l + 1));
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

    // MGET	获取所有(一个或多个)给定 key 的值
    std::string mgetCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'mget' command");
        }

        std::string res = "*" + std::to_string(args.size() - 1) + "\r\n";

        for (size_t i = 1; i < args.size(); ++i) {
            const std::string &key = args[i];
            Object *obj = db.get(key);

            if (!obj || obj->type != ObjType::STRING) {
                res += protocol::Encoder::encodeNull();
            } else {
                res += protocol::Encoder::encodeBulkString(std::get<GString>(obj->value));
            }
        }
        return res;
    }

    // GETBIT	对 key 所储存的字符串值，获取指定偏移量上的位 ( bit )
    // SETBIT	对 key 所储存的字符串值，设置或清除指定偏移量上的位(bit)

    // STRLEN key
    std::string strlenCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'strlen' command");
        }

        const std::string &key = args[1];
        Object *obj = db.get(key);
        if (!obj) return protocol::Encoder::encodeInteger(0);
        if (obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        return protocol::Encoder::encodeInteger(std::get<GString>(obj->value).size());
    }

    // MSET key value [key value ...]
    std::string msetCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 3 || args.size() % 2 == 0) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'mset' command");
        }

        for (size_t i = 1; i < args.size(); i += 2) {
            db.set(args[i], Object(args[i + 1]));
        }

        return protocol::Encoder::encodeSimpleString("OK");
    }

    // SETEX key seconds value
    std::string setexCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'setex' command");
        }

        long long seconds;
        const std::string &secStr = args[2];
        auto [ptr, ec] = std::from_chars(secStr.data(), secStr.data() + secStr.size(), seconds);

        if (ec != std::errc{} || ptr != secStr.data() + secStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        if (seconds <= 0) {
            return protocol::Encoder::encodeError("ERR invalid expire time in 'setex' command");
        }

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        db.set(args[1], Object(args[3], now + seconds * 1000));
        return protocol::Encoder::encodeSimpleString("OK");
    }

    // SETNX key value
    std::string setnxCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'setnx' command");
        }

        if (db.exists(args[1])) {
            return protocol::Encoder::encodeInteger(0);
        }

        db.set(args[1], Object(args[2]));
        return protocol::Encoder::encodeInteger(1);
    }

    // MSETNX key value [key value ...]
    std::string msetnxCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() < 3 || args.size() % 2 == 0) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'msetnx' command");
        }
        for (size_t i = 1; i < args.size(); i += 2) {
            if (db.exists(args[i])) return protocol::Encoder::encodeInteger(0);
        }
        for (size_t i = 1; i < args.size(); i += 2) {
            db.set(args[i], Object(args[i + 1]));
        }
        return protocol::Encoder::encodeInteger(1);
    }

    // PSETEX key milliseconds value
    std::string psetexCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 4) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'psetex' command");
        }

        long long ms;
        const std::string &msStr = args[2];
        auto [ptr, ec] = std::from_chars(msStr.data(), msStr.data() + msStr.size(), ms);

        if (ec != std::errc{} || ptr != msStr.data() + msStr.size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        if (ms <= 0) {
            return protocol::Encoder::encodeError("ERR invalid expire time in 'psetex' command");
        }

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        db.set(args[1], Object(args[3], now + ms));

        return protocol::Encoder::encodeSimpleString("OK");
    }

    // SETRANGE	从偏移量 offset 开始用 value 覆写给定 key 所储存的字符串值

    // INCR key - 对 key 中储存的数字值增一
    std::string incrCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'incr' command");
        }

        const std::string &key = args[1];
        Object *obj = db.get(key);

        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (!obj) {
            db.set(key, Object(std::string("0")));
            obj = db.get(key);
        }

        return doIncrLike(std::get<GString>(obj->value), 1);
    }

    // INCRBY	将 key 所储存的值加上给定的增量值
    std::string incrbyCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'incrby' command");
        }

        const std::string &key = args[1];

        // 解析增量
        int64_t delta;
        auto [ptr, ec] = std::from_chars(args[2].data(), args[2].data() + args[2].size(), delta);
        if (ec != std::errc{} || ptr != args[2].data() + args[2].size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        Object *obj = db.get(key);

        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (!obj) {
            db.set(key, Object(std::string("0")));
            obj = db.get(key);
        }

        return doIncrLike(std::get<GString>(obj->value), delta);
    }

    // DECR	将 key 中储存的数字值减一
    std::string decrCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'decr' command");
        }

        const std::string &key = args[1];
        Object *obj = db.get(key);

        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (!obj) {
            db.set(key, Object(std::string("0")));
            obj = db.get(key);
        }

        return doIncrLike(std::get<GString>(obj->value), -1);
    }

    // DECRBY	将 key 所储存的值减去给定的减量值
    std::string decrbyCommand(std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'decrby' command");
        }

        const std::string &key = args[1];

        // 解析减量
        int64_t delta;
        auto [ptr, ec] = std::from_chars(args[2].data(), args[2].data() + args[2].size(), delta);
        if (ec != std::errc{} || ptr != args[2].data() + args[2].size()) {
            return protocol::Encoder::encodeError("ERR value is not an integer or out of range");
        }

        Object *obj = db.get(key);

        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (!obj) {
            db.set(key, Object(std::string("0")));
            obj = db.get(key);
        }

        return doIncrLike(std::get<GString>(obj->value), -delta);
    }

    // INCRBYFLOAT	将 key 所储存的值加上给定的浮点增量值
    // APPEND	将 value 追加到 key 原来的值的末尾
    std::string appendCommand(const std::vector<std::string> &args, Database &db) {
        if (args.size() != 3) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'append' command");
        }
        const std::string &key = args[1], &value = args[2];

        Object *obj = db.get(key);
        if (obj && obj->type != ObjType::STRING) {
            return protocol::Encoder::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        }

        if (obj) {
            auto &str = std::get<GString>(obj->value);
            str.append(value);
            return protocol::Encoder::encodeInteger(str.size());
        }

        db.set(key, Object(value));
        return protocol::Encoder::encodeInteger(value.size());
    }

    // 自注册
    static AutoRegister reg_set("SET", setCommand);
    static AutoRegister reg_get("GET", getCommand);
    static AutoRegister reg_getrange("GETRANGE", getrangeCommand);
    static AutoRegister reg_getset("GETSET", getsetCommand);
    static AutoRegister reg_mget("MGET", mgetCommand);
    static AutoRegister reg_strlen("STRLEN", strlenCommand);
    static AutoRegister reg_mset("MSET", msetCommand);
    static AutoRegister reg_setex("SETEX", setexCommand);
    static AutoRegister reg_setnx("SETNX", setnxCommand);
    static AutoRegister reg_msetnx("MSETNX", msetnxCommand);
    static AutoRegister reg_psetex("PSETEX", psetexCommand);
    static AutoRegister reg_append("APPEND", appendCommand);
    static AutoRegister reg_incr("INCR", incrCommand);
    static AutoRegister reg_incrby("INCRBY", incrbyCommand);
    static AutoRegister reg_decr("DECR", decrCommand);
    static AutoRegister reg_decrby("DECRBY", decrbyCommand);
} // namespace gudb::cmd

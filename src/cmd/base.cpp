#include "Registry.h"
#include "../protocol/Encoder.h"
#include <chrono>
#include <vector>
#include <string>

namespace gudb::cmd {
    // PING
    std::string pingCommand([[maybe_unused]] const std::vector<std::string> &args, [[maybe_unused]] Database &db) {
        return protocol::Encoder::encodeSimpleString("PONG");
    }

    // ECHO
    std::string echoCommand(const std::vector<std::string> &args, [[maybe_unused]] Database &db) {
        if (args.size() != 2) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'echo' command");
        }
        return protocol::Encoder::encodeBulkString(args[1]);
    }

    // TIME
    std::string timeCommand([[maybe_unused]] const std::vector<std::string> &args, [[maybe_unused]] Database &db) {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        return protocol::Encoder::encodeArray({
            std::to_string(us / 1'000'000),
            std::to_string(us % 1'000'000)
        });
    }

    // FLUSHDB
    std::string flushDBCommand([[maybe_unused]] const std::vector<std::string> &args, Database &db) {
        if (args.size() != 1) {
            return protocol::Encoder::encodeError("ERR wrong number of arguments for 'flushdb' command");
        }
        db.clear();
        return protocol::Encoder::encodeSimpleString("OK");
    }

    //自注册
    static AutoRegister reg_ping("PING", pingCommand);
    static AutoRegister reg_echo("ECHO", echoCommand);
    static AutoRegister reg_time("TIME", timeCommand);
    static AutoRegister reg_flushdb("FLUSHDB", flushDBCommand);
}

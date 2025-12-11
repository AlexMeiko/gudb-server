#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../db/Database.h"

namespace gudb::cmd {
    class Registry {
    public:
        using CommandFunc = std::function<std::string(std::vector<std::string> &, Database &)>;

        static Registry &instance();

        void registerCommand(const std::string &name, CommandFunc func);

        CommandFunc getCommand(const std::string &name);

    private:
        Registry() = default;

        std::unordered_map<std::string, CommandFunc> commands_;
    };

    // 自动注册器：在静态对象构造阶段将命令注册到注册表（RAII）
    struct AutoRegister {
        AutoRegister(const std::string &name, Registry::CommandFunc func) {
            Registry::instance().registerCommand(name, std::move(func));
        }
    };
} // namespace gudb::cmd

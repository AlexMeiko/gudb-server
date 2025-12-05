#include "Registry.h"
#include <utility>
#include <string>

namespace gudb::cmd {
    Registry &Registry::instance() {
        static Registry instance;
        return instance;
    }

    void Registry::registerCommand(const std::string &name, CommandFunc func) {
        commands_[name] = std::move(func);
    }

    Registry::CommandFunc Registry::getCommand(const std::string &name) {
        auto it = commands_.find(name);
        if (it != commands_.end()) {
            return it->second;
        }
        return nullptr;
    }
} // namespace gudb::cmd
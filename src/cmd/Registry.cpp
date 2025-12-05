#include "Registry.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace gudb::cmd {
    Registry &Registry::instance() {
        static Registry instance;
        return instance;
    }

    void Registry::registerCommand(const std::string &name, CommandFunc func) {
        std::string upper;
        upper.reserve(name.size());
        for (unsigned char c: name) {
            upper.push_back(static_cast<char>(std::toupper(c)));
        }

        commands_[std::move(upper)] = std::move(func);
    }

    Registry::CommandFunc Registry::getCommand(const std::string &name) {
        std::string upper;
        upper.reserve(name.size());
        for (unsigned char c: name) {
            upper.push_back(static_cast<char>(std::toupper(c)));
        }

        auto it = commands_.find(upper);
        return it != commands_.end() ? it->second : nullptr;
    }
} // namespace gudb::cmd
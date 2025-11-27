#pragma once
#include <vector>
#include <string>
#include "../db/Database.h"

namespace gudb::cmd {
    class Command {
    public:
        virtual ~Command() = default;

        virtual std::string execute(const std::vector<std::string> &args, Database &db) = 0;
    };
} // namespace gudb::cmd

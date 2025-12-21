#include <csignal>
#include <iostream>
#include "db/Database.h"
#include "net/Server.h"

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    gudb::Database db;
    gudb::net::Server server(&db);
    if (!server.listen("0.0.0.0", 6378)) {
        std::cerr << "Failed to start gudb-server: listen failed on 0.0.0.0:6378" << std::endl;
        return 1;
    }
    server.run();
    return 0;
}

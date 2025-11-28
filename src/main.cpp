#include "net/Server.h"
#include "db/Database.h"

int main() {
    gudb::Database db;
    gudb::net::Server server(&db);
    server.listen("0.0.0.0", 6378);
    server.run();
    return 0;
}
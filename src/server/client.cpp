#include "client.h"
#include "../common/debug.h"

Client::Client(Connection c):connection(c) {;}

bool Client::execute(void) {
    char buffer[12];

    connection.send("CiaoCiaoCiao", 12);
    connection.recv(buffer, 12);

    debug(DEBUG, "client execution end");
    return true;
}

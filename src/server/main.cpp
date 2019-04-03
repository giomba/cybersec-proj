#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/connection.h"
#include "../common/debug.h"
#include "server.h"

int main(int argc, char** argv) {

    Server server = Server("::0", 4242);

    while (true) {
        Connection client = server.accept();
        // add client to list??
        client.send("Hello world!", 12);
        debug(INFO, "data sent");
        // close(client_sd); // TODO remember to implement socket close in the destructor of Connection
    }

    return 0;
}

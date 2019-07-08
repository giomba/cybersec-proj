#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "client.h"
#include "server.h"

using namespace std;

int main(int argc, char** argv) {

    try {
        /* check for a valid number of arguments */
        if (argc != 2 && argc != 3) throw ExUserInput("invalid number of arguments");

        /* parse port */
        uint16_t port = atoi(argv[1]);

        /* enable debug output */
        if (argc == 3) debugenable(argv[2]);

        Server server = Server("::0", port);

        Client* client = NULL;

        while (true) {
            client = new Client(server.accept(), server.getCertManager());

            thread t(&Client::execute, client);

            debug(INFO, "[I] thread created for client " << client << endl);

            t.detach();

            /* Client will delete itself on its own when its execution finishes */
        }
    } catch (ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    } catch (ExUserInput e) {
        cerr << "[E] invalid input: " << e << endl;
    }

    return 0;
}

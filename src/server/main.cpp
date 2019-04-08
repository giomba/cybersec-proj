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

#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "check.h"
#include "client.h"
#include "server.h"

using namespace std;

int main(int argc, char** argv) {

    try {

        if (argc != 2) throw ExUserInput("invalid number of arguments");
        uint16_t port = atoi(argv[1]);
        if (! checkPort(port)) throw ExUserInput("invalid port number");

        Server server = Server("::0", atoi(argv[1]));
        // debug(INFO, string("Listening on port ")); debug(INFO);

        while (true) {
            Client client = Client(server.accept());

            // TODO: create a list of threads and answer them
            thread t(&Client::execute, &client);

            debug(INFO, "thread created");

            t.detach();

            // TODO remember to implement socket close in the destructor of Connection
        }
    } catch (ExNetwork e) {
        cerr << "network: " << e << endl;
    } catch (ExUserInput e) {
        cerr << "invalid input: " << e << endl;
    }

    return 0;
}

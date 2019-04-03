#include <arpa/inet.h>
#include <errno.h>
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
#include "client.h"
#include "server.h"

using namespace std;

int main(int argc, char** argv) {

    Server server = Server("::0", 4242);

    while (true) {
        Client client = Client(server.accept());

        // TODO: create a list of threads and answer them
        thread t(&Client::execute, &client);

        debug(INFO, "thread created");

        t.detach();

        // TODO remember to implement socket close in the destructor of Connection
    }

    return 0;
}

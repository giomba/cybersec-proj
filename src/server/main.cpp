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

        Client* client = NULL;

        while (true) {
            try {
                client = new Client(server.accept());

                thread t(&Client::execute, client);

                debug(INFO, "[I] thread created for client " << client << endl);

                t.detach();

                // TODO remember to destroy these clients when they disconnect
            }
            catch (ExProtocol e) {
                delete client;
                debug(ERROR, "[E] protocol: " << e << endl);
                continue;
            }
            catch (ExCertificate e) {
                delete client;
                debug(ERROR, "[E] certificate: " << e << endl);
                continue;
            }

        }
    } catch (ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    } catch (ExUserInput e) {
        cerr << "[E] invalid input: " << e << endl;
    }

    return 0;
}

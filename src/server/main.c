#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int sd = 0;
    struct sockaddr_in6 my_addr, client_addr;
    socklen_t sizeof_addr;

    sizeof_addr = sizeof(client_addr);

    sd = socket(AF_INET6, SOCK_STREAM, 0);

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin6_family = AF_INET6;
    my_addr.sin6_port = htons(4242);
    my_addr.sin6_flowinfo = 0;
    my_addr.sin6_scope_id = 0;
    inet_pton(AF_INET6, "::0", &my_addr.sin6_addr);


    if ( bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr)) != 0 ) {
        perror("bind()");
        exit(1);
    }

    if ( listen(sd, 10) != 0 ) {    // TODO: choose a proper number
        perror("listen()");
        exit(1);
    }

    int client_sd;
    char buffer[] = "Hello world!";

    while (1) {
        client_sd = accept(sd, (struct sockaddr*)&client_addr, &sizeof_addr);
        send(client_sd, (void*)buffer, strlen(buffer), 0);
        close(client_sd);
    }

    return 0;
}

#include <iostream>
using namespace std;

#include "../common/connection.h"

int main(int argc, char** argv) {
    char buffer[128];

    Connection c = Connection("::1", 4242);

    c.recv(buffer, 5);
    buffer[127] = '\0';

    cout << buffer << endl;

    return 0;
}

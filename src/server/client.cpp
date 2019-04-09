#include <iostream>
#include <sstream>
#include <vector>

#include "../common/connection.h"
#include "../common/debug.h"

#include "client.h"

using namespace std;

#define BUFFER_SIZE 4 * (1 << 10)
#define SERVER_ROOT "root"

Client::Client(Connection* c) {
    connection = c;
}

void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
    int recvBytes;
    char shiftRegister[2];

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = connection->recv(buffer + i, 1);
        if (recvBytes == 1) {
            shiftRegister[0] = shiftRegister[1];
            shiftRegister[1] = buffer[i];

            // if all command and parameters have been received...
            if (shiftRegister[0] == '\n' && shiftRegister[1] == '\n') {
                buffer[i + 1] = '\0';
                break;
            }
        }
    }
    buffer[BUFFER_SIZE - 1] = '\0';

    arg.clear();

    istringstream stream;
    stream.str(string(buffer));

    string parola;
    while (stream >> parola && stream.good()) {
        arg.push_back(parola);
    }
}

void Client::sendCmd() {
    // server must always reply with an odd number of parolas in the ReplyStatus
    assert(reply.size() % 2 == 1);

    connection->send(reply[0].c_str(), reply[0].length());
    connection->send("\n", 1);

    for (unsigned int i = 1; i < reply.size(); i += 2) {
        connection->send(reply[i].c_str(), reply[i].length());
        connection->send(" ", 1);
        connection->send(reply[i + 1].c_str(), reply[i + 1].length());
        connection->send("\n", 1);
    }
    connection->send("\n", 1);
}

void Client::cmd_allo(void) {;}
void Client::cmd_dele(void) {;}

void Client::cmd_list(void) {
    /* check for syntax errors */
    if (arg.size() != 1) {
        reply.push_back(SYNTAX_ERROR);
        sendCmd();
        return;
    }

    /* retrieve list of files and computer size */
    vector<string> fileList;
    size_t size = 0;
    DIR* dd;
    dirent* de;

    dd = opendir(SERVER_ROOT);
    if (dd) {
        while ((de = readdir(dd)) != NULL) {
            fileList.push_back(string(de->d_name));
            size += (strlen(de->d_name) + 1);
        }
        closedir(dd);
    }
    else {
        reply.push_back(SERVER_ERROR);
        sendCmd();
        return;
    }

    /* send back list of files */
    reply.push_back(OK);
    reply.push_back(string("Size: "));
    reply.push_back(to_string(size));
    sendCmd();

    for (string filename : fileList) {
        connection->send(filename.c_str(), filename.length());
        connection->send("\n", 1);
    }
}

void Client::cmd_quit(void) {;}
void Client::cmd_retr(void) {;}
void Client::cmd_stor(void) {;}
void Client::cmd_unknown(void) {
    reply.push_back(COMMAND_NOT_IMPLEMENTED);
    sendCmd();
}

bool Client::execute(void) {
    for (;;) {
        reply.clear();

        recvCmd();

        if (arg.size() >= 1) {
                 if (arg[0] == "ALLO") { cmd_allo(); }
            else if (arg[0] == "DELE") { cmd_dele(); }
            else if (arg[0] == "LIST") { cmd_list(); }
            else if (arg[0] == "QUIT") { cmd_quit(); }
            else if (arg[0] == "RETR") { cmd_retr(); }
            else if (arg[0] == "STOR") { cmd_stor(); }
            else cmd_unknown();
        }
    }

    debug(DEBUG, "client execution end");
    return true;
}

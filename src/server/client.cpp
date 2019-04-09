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

    is.str(string(buffer));
}

void Client::sendCmd() {
    string buffer = os.str();

    connection->send(buffer.c_str(), buffer.length());
}

void Client::cmd_allo(void) {;}
void Client::cmd_dele(void) {;}

void Client::cmd_list(void) {
    /* retrieve list of files */
    ostringstream fileList;
    DIR* dd;
    dirent* de;

    dd = opendir(SERVER_ROOT);
    if (dd) {
        while ((de = readdir(dd)) != NULL) {
            if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) { // if name is not . nor ..
                fileList << de->d_name << endl;
            }
        }
        closedir(dd);
    }
    else {
        os << SERVER_ERROR << endl;
        sendCmd();
        return;
    }

    /* send back list of files */
    string buffer = fileList.str();

    os << OK << endl << "Size: " << buffer.length() << endl << endl;
    sendCmd();

    connection->send(buffer.c_str(), buffer.length());

}

void Client::cmd_quit(void) {;}
void Client::cmd_retr(void) {;}
void Client::cmd_stor(void) {;}
void Client::cmd_unknown(void) {
    os << COMMAND_NOT_IMPLEMENTED << endl;
    sendCmd();
}

bool Client::execute(void) {
    string cmd;

    try {

        for (;;) {
            /* Clear Input and Output streams */
            is.ignore(BUFFER_SIZE);
            is.clear();
            os.str("");
            os.clear();

            /* Receive command and read first parola */
            recvCmd();
            is >> cmd;

            /* Choose command function */
            if (is.good()) {
                     if (cmd == "ALLO") { cmd_allo(); }
                else if (cmd == "DELE") { cmd_dele(); }
                else if (cmd == "LIST") { cmd_list(); }
                else if (cmd == "QUIT") { cmd_quit(); }
                else if (cmd == "RETR") { cmd_retr(); }
                else if (cmd == "STOR") { cmd_stor(); }
                else { cmd_unknown(); }
            }
        }
    }
    catch (ExNetwork e) {
        cerr << e << endl;
    }
    catch (Ex e) {
        cerr << e << endl;
    }

    clog << "[II] client execution end " << this << endl;
    return true;
}


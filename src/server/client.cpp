#include "../common/connection.h"
#include "../common/debug.h"

#include "client.h"

using namespace std;

#define BUFFER_SIZE 4 * (1 << 10)
const string SERVER_ROOT = "root";

Client::Client(Connection* c) {
    connection = c;
}

void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
    int recvBytes;
    char shiftRegister[2];

    is.ignore(BUFFER_SIZE);
    is.clear();


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

int Client::recvBodyFragment(char* buffer, int len) {
    return connection->recv(buffer, len);
}

void Client::sendCmd() {
    string buffer = os.str();
    if (buffer.size() != 0) {
        connection->send(buffer.data(), buffer.size());
    }

    os.str("");
    os.clear();
}

void Client::cmd_dele(void) {
    string filename;
    is >> filename;

    if (! regex_match(filename, parola)) {
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    string fullpath = SERVER_ROOT + "/" + filename;

    if (remove(fullpath.c_str()) == 0) {
        os << OK << endl << endl;
        sendCmd();
    }
    else {
        clog << "[W] can not remove file ->" << fullpath << "<-" << endl;
        os << BAD_FILE << endl;
        sendCmd();
    }
}

void Client::cmd_list(void) {
    /* retrieve list of files */
    ostringstream fileList;
    DIR* dd;
    dirent* de;

    dd = opendir(SERVER_ROOT.c_str());
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
    os.write(buffer.data(), buffer.size());
    sendCmd();
}

void Client::cmd_quit(void) {
    os << OK << endl << endl;
    sendCmd();
}

void Client::cmd_retr(void) {
    string filename;

    is >> filename;

    if (! regex_match(filename, parola)) {
        clog << "[W] bad file name" << endl;
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    string fullpath = SERVER_ROOT + "/" + filename;
    clog << "[I] RETR file ->" << fullpath << "<-" << endl;
    fstream file(fullpath, ios::in | ios::binary);

    if (! file) {
        clog << "[W] file can not be open" << endl;
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    file.seekg(0, ios::end);
    int size = file.tellg();

    os << OK << endl << "Size: " << size << endl << endl;
    sendCmd();

    char c;
    int count = 0;
    file.seekg(0, ios::beg);
    while (true) {
        c = file.get();
        if (! file.good()) break;
        os << c;
        if (count++ % BUFFER_SIZE == 0) {
            sendCmd();
        }
    }
    sendCmd();

    file.close();
}

void Client::cmd_stor(void) {
    string filename;
    is >> filename;

    if (! regex_match(filename, parola)) {
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    string tag;
    int size;

    is >> tag >> size;

    if (tag != "Size:") {
        os << SYNTAX_ERROR << endl << endl;
        sendCmd();
        return;
    }

    string fullpath = SERVER_ROOT + "/" + filename;
    clog << "[I] STOR ->" << fullpath << "<-" << endl;

    fstream file(fullpath, ios::out | ios::binary);
    if (! file) {
        clog << "[W] can not open " << filename << endl;
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    os << OK << endl << endl;
    sendCmd();

    char buffer[BUFFER_SIZE];

    int fragmentSize, remaining, received = 0;

    while (received < size) {
        remaining = size - received;
        fragmentSize = recvBodyFragment(buffer, (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining);
        file.write(buffer, fragmentSize);
        received += fragmentSize;
    }
    clog << "[I] received " << received << " bytes for " << fullpath << endl;
    file.close();
}

void Client::cmd_unknown(void) {
    os << COMMAND_NOT_IMPLEMENTED << endl;
    sendCmd();
}

bool Client::execute(void) {
    string cmd;

    try {

        for (;;) {
            /* Receive command and read first parola */
            recvCmd();
            is >> cmd;

            /* Choose command function */
            if (is.good()) {
                     if (cmd == "DELE") { cmd_dele(); }
                else if (cmd == "LIST") { cmd_list(); }
                else if (cmd == "QUIT") { cmd_quit(); break; }
                else if (cmd == "RETR") { cmd_retr(); }
                else if (cmd == "STOR") { cmd_stor(); }
                else { cmd_unknown(); }
            }
        }
    }
    catch (ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    }
    catch (Ex e) {
        cerr << "[E] " << e << endl;
    }

    clog << "[I] client execution end " << this << endl;

    delete connection;

    return true;
}


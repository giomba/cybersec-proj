#include "client.h"

const string SERVER_ROOT = "root";

Client::Client(Connection* c) : crypto((const unsigned char*)"0123456789abcdef", (const unsigned char*)"fedcba9876543210", (const unsigned char*)"0000000000000000") {
    connection = c;
}

void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
//    char ciphertext;
    int recvBytes;
    char shiftRegister[2];

    is.ignore(BUFFER_SIZE);
    is.clear();

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = crypto.recv(connection, buffer + i, 1);
        //recvBytes = connection->recv(&ciphertext, 1);
        //crypto.decrypt(buffer + i, &ciphertext, 1);
        //recvBytes = connection->recv(buffer + i, 1);
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

    // BIO_dump_fp(stdout, (const char*)buffer, strlen(buffer));

    is.str(string(buffer));
}

int Client::recvBodyFragment(char* buffer, const int len) {
//    char ciphertext[BUFFER_SIZE];
    int r;

    r = crypto.recv(connection, buffer, len);
    // clog << "[D] Received a fragment of " << r << " bytes" << endl;
    // crypto.decrypt(buffer, ciphertext, r);

    /*
    clog << "[D] ciphertext" << endl;
    BIO_dump_fp(stdout, (const char*)ciphertext, (r > 32) ? 32 : r);
    clog << "[D] plaintext" << endl;
    BIO_dump_fp(stdout, (const char*)buffer, (r > 32) ? 32 : r);
    */

    return r;
}

void Client::sendCmd() {
    string buffer = os.str();
    if (buffer.size() != 0) {

        // compute sequence number
        // compute hmac

/*        string ciphertext;
        ciphertext.resize(buffer.size());*/

        crypto.send(connection, &buffer[0], buffer.size());

//        crypto.encrypt(&ciphertext[0], &buffer[0], buffer.size());
//        connection->send(ciphertext.data(), ciphertext.size());
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
        clog << "[I] delete file ->" << fullpath << "<-" << endl;
        os << OK << endl << endl;
        sendCmd();
    }
    else {
        clog << "[W] can not remove file ->" << fullpath << "<-" << endl;
        os << BAD_FILE << endl << endl;
        sendCmd();
    }
}

void Client::cmd_list(void) {
    clog << "[I] Sending directory list" << endl;
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
        os << SERVER_ERROR << endl << endl;
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
    int64_t size = file.tellg();

    os << OK << endl << "Size: " << size << endl << endl;
    sendCmd();

    char c;
    int64_t count = 0;
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
    int64_t size;

    is >> tag >> size;
    clog << "[D] " << size << endl;

    if (tag != "Size:") {
        os << SYNTAX_ERROR << endl << endl;
        sendCmd();
        return;
    }

    if (size > MAX_FILE_SIZE) {
        os << COMMAND_NOT_IMPLEMENTED << endl << endl;
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

    int64_t fragmentSize, remaining, received = 0;

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
    clog << "[W] client issued not implemented command" << endl;
    os << COMMAND_NOT_IMPLEMENTED << endl << endl;
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
                else { clog << "[W] ->" << cmd << "<-" << endl; cmd_unknown(); }
            }
            else {
                clog << "[W] bad input stream" << endl;
            }
        }

        clog << "[I] client " << this << " quit" << endl;
    }
    catch (ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    }
    catch (Ex e) {
        cerr << "[E] " << e << endl;
    }

    delete connection;

    return true;
}


#include "client.h"

const string SERVER_ROOT = "root";

Client::Client(Connection* c, CertManager* cm) {
    this->connection = c;
    this->cm = cm;
    this->crypto = 0;
}

Client::~Client() {
    if (this->connection) delete connection;
    if (this->crypto) delete crypto;
}


void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
    int recvBytes;
    char shiftRegister[2];

    memset(buffer, 0, BUFFER_SIZE);
    memset(shiftRegister, 0, 2);

    is.ignore(BUFFER_SIZE);
    is.clear();

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = crypto->recv(connection, buffer + i, 1);
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

int Client::recvBodyFragment(char* buffer, const int len) {
    int r;

    r = crypto->recv(connection, buffer, len);

    debug(DEBUG, "[D] fragment plaintext" << endl);
    hexdump(DEBUG, (const char*)buffer, r);

    return r;
}

void Client::sendCmd() {
    string buffer = os.str();
    if (buffer.size() != 0) {
        crypto->send(connection, &buffer[0], buffer.size());
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
        debug(INFO, "[I] delete file ->" << fullpath << "<-" << endl);
        os << OK << endl << endl;
        sendCmd();
    }
    else {
        debug(WARNING, "[W] can not remove file ->" << fullpath << "<-" << endl);
        os << BAD_FILE << endl << endl;
        sendCmd();
    }
}

void Client::cmd_list(void) {
    debug(INFO, "[I] Sending directory list" << endl);
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
        debug(WARNING, "[W] bad file name" << endl);
        os << BAD_FILE << endl << endl;
        sendCmd();
        return;
    }

    string fullpath = SERVER_ROOT + "/" + filename;
    debug(INFO, "[I] RETR file ->" << fullpath << "<-" << endl);
    fstream file(fullpath, ios::in | ios::binary);

    if (! file) {
        debug(WARNING, "[W] file can not be open" << endl);
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
    debug(DEBUG, "[D] " << size << endl);

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
    debug(INFO, "[I] STOR ->" << fullpath << "<-" << endl);

    fstream file(fullpath, ios::out | ios::binary);
    if (! file) {
        debug(WARNING, "[W] can not open " << filename << endl);
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
    debug(INFO, "[I] received " << received << " bytes for " << fullpath << endl);
    file.close();
}

void Client::cmd_unknown(void) {
    debug(WARNING, "[W] client issued not implemented command" << endl);
    os << COMMAND_NOT_IMPLEMENTED << endl << endl;
    sendCmd();
}

int Client::handshake(void) {
    debug(INFO, "[I] handshake with client..." << endl);
    /* === M1 === */
    string buffer;

    connection->recv(buffer);
    debug(DEBUG, "[D] serialized certificate" << endl);
    vstrdump(DEBUG, buffer);
    Certificate certificate(buffer);
    cm->verifyCert(certificate);

    //delete certificate;
    /* if (some error) return -1; else */
    return 0;
}

bool Client::execute(void) {
    string cmd;
    /* TODO
    unsigned char* sessionKey;
    unsigned char* authKey;
    unsigned char* iv;
    */

    string sessionKey = "1230000000000321";
    string authKey = "0123456789abcdef9876543210abcdef";
    string iv = "0000000000000000";

    try {
        /* key exchange handshake */
        //handshake(session_key, auth_key, iv);
        handshake();
        this->crypto = new Crypto(sessionKey, authKey, iv);
        //this->signer = new Signer();

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
                else { debug(WARNING, "[W] ->" << cmd << "<-" << endl); cmd_unknown(); }
            }
            else {
                debug(WARNING, "[W] bad input stream" << endl);
            }
        }

        debug(INFO, "[I] client " << this << " quit" << endl);
    }
    catch (ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    }
    catch (ExCertificate e) {
        cerr << "[E] certificate: " << e << endl;
    }
    catch (ExProtocol e) {
        cerr << "[E] protocol: " << e << endl;
    }
    catch (Ex e) {
        cerr << "[E] " << e << endl;
    }

    delete this;

    return true;
}

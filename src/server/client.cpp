#include "client.h"

const string SERVER_ROOT = "root";

Client::Client(Connection* c, CertManager* cm) {
    this->connection = c;
    this->cm = cm;
    this->crypto = NULL;
}

Client::~Client() {
    if (this->connection != NULL) delete connection;
    if (this->crypto != NULL) delete crypto;
}


void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
    int recvBytes = 0;
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

vector<Key> Client::handshake(void) {
    string buffer;
    debug(INFO, "[I] handshake with client..." << endl);

    /* === M1 === */
    /* receive client's certificate */
    connection->recv(buffer);
    debug(DEBUG, "[D] Client Certificate" << endl); vstrdump(DEBUG, buffer);
    Certificate client_certificate; client_certificate.fromString(buffer);
    cm->verifyCert(client_certificate);

    /* receive client's nonce */
    connection->recv(buffer);
    debug(DEBUG, "[D] Client Nonce" << endl); vstrdump(DEBUG, buffer);
    Nonce nonceClient(buffer);

    /* === M2 === */
    /* initialize RSA Crypto */
    RSACrypto(cm->getCert(), cm->getPrivKey());
    /* prepare server's certificate */
    Certificate& server_certificate = cm->getCert();
    debug(DEBUG, "[D] Server Certificate" << endl); vstrdump(DEBUG, server_certificate.str());

    /* generate keys and initialization vector */
    Key session_key(AES128_KEY_LEN);
    Key auth_key(HMAC_KEY_LEN);
    Key iv(AES128_KEY_LEN);
    debug(DEBUG, "[D] Ks" << endl); vstrdump(DEBUG, session_key.str());
    debug(DEBUG, "[D] Ka" << endl); vstrdump(DEBUG, auth_key.str());
    debug(DEBUG, "[D] IV" << endl); vstrdump(DEBUG, iv.str());
    /* generate server's nonce */
    Nonce nonceServer;

    /* encrypt keys */
    RSAKey client_public_key; client_public_key.fromCertificate(client_certificate);
    RSACrypto rsacrypto(cm->getCert(), cm->getPrivKey());
    string session_key_serialized = session_key.str();
    string auth_key_serialized = auth_key.str();

    RSASeal encrypted_session_key_seal = rsacrypto.encrypt(session_key_serialized, client_public_key);
    RSASeal encrypted_auth_key_seal = rsacrypto.encrypt(auth_key_serialized, client_public_key);

    /* sign message */
    string what_to_sign = encrypted_session_key_seal.str() + encrypted_auth_key_seal.str() + iv.str() + nonceClient.str();
    string signature = rsacrypto.sign(what_to_sign);

    /* send everything */
    buffer = server_certificate.str();          connection->send(buffer);
    buffer = encrypted_session_key_seal.str();  connection->send(buffer);
    buffer = encrypted_auth_key_seal.str();     connection->send(buffer);
    buffer = iv.str();                          connection->send(buffer);
    buffer = signature;                         connection->send(buffer);
    buffer = nonceServer.str();                 connection->send(buffer);

    /* === M3 === */
    /* receive server's nonce's signature, and verify */
    connection->recv(signature);
    string what_to_verify = nonceServer.str();
    rsacrypto.verify(what_to_verify, signature, client_public_key);

    /* if execution arrives here without any exception thrown, then handshake is finished well! */
    debug(DEBUG, "[D] Successfull handshake!" << endl);

    /* return generated session keys */
    vector<Key> keys;
    keys.push_back(session_key);
    keys.push_back(auth_key);
    keys.push_back(iv);
    return keys;
}

void Client::execute(void) {
    string cmd;

    try {
        /* key exchange handshake */
        vector<Key> keys = handshake();
        /* session_key, auth_key, iv */
        this->crypto = new Crypto(keys.at(0), keys.at(1), keys.at(2));

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
    catch (bad_alloc e) {
        cerr << "[E] bad_alloc: out of memory" << endl;
    }

    /* This member function is executing as a thread, and this Client has
        no reason to stay alive after this function terminates its execution, so
        «as long as we are careful, it is fine to commit suicide» */
    delete this;
}

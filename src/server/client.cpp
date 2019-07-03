#include "client.h"

const string SERVER_ROOT = "root";

Client::Client(Connection* c, CertManager* cm) {
    this->connection = c;
    this->cm = cm;
}

Client::~Client() {
    if (this->connection) delete connection;
    if (this->crypto) delete crypto;
}

int Client::handshake() {
    receiveM1();
    sendM2();
    //receiveM3();
}

int Client::receiveM1() {
    /* receive M1 */
    M1 m1;
    connection->recv((char*)&m1, sizeof(m1));

    /* if client sends an exagerated size for certificate or signature, don't allocate the buffers */
    m1.certLen = ntohl(m1.certLen);
    m1.signLen = ntohl(m1.signLen);
    if (m1.certLen > BUFFER_SIZE) throw ExTooBig("client certificate too big");
    if (m1.signLen > BUFFER_SIZE) throw ExTooBig("nonce signature too big");

    debug(DEBUG, "[D] certLen: " << m1.certLen << "\t" << "signLen: " << m1.signLen << endl);

    unsigned char* serialized_client_certificate = new unsigned char[m1.certLen];
    connection->recv((char*)serialized_client_certificate, m1.certLen);
    unsigned char* client_signature = new unsigned char[m1.signLen];
    connection->recv((char*)client_signature, m1.signLen);

    debug(DEBUG, "[D] received M1 + payload" << endl);
    hexdump(DEBUG, (const char*)&m1, sizeof(m1));
    hexdump(DEBUG, (const char*)serialized_client_certificate, m1.certLen);
    hexdump(DEBUG, (const char*)client_signature, m1.signLen);

    /* deserialize certificate */
    X509* client_certificate = d2i_X509(NULL, (const unsigned char**)&serialized_client_certificate, m1.certLen);
    if (!client_certificate){
        debug(ERROR, "cannot deserialize client certificate on socket " << connection->getSocket() << endl);
        return -1;
    }

    /* check validity */
    if (cm->verifyCert(client_certificate, "") == -1) {
        debug(ERROR, "[E] client is not authenticated by TrustedCA" << endl);
        throw ExCertificate("client is not authenticated by TrustedCA");
    }
    debug(INFO, "[I] client on socket " << connection->getSocket() << " is authenticated" << endl);

    /* verify nonce signature */
    if (cm->verifySignature(client_certificate, (char*)&(m1.nonceC), sizeof(m1.nonceC), client_signature, m1.signLen) == -1) {
        debug(ERROR, "[E] client's nonce signature is not valid" << endl);
        throw ExCertificate("client nonce signature is not valid");
    }
    debug(INFO, "[I] valid nonce for client socket " << connection->getSocket() << endl);

    return 0;

    /* free memory */
    // TODO
    //delete[] client_signature;
    //delete[] serialized_client_certificate;

}

int Client::sendM2() {
    /* serialize certificate */
    X509* server_certificate = cm->getCert();
    int server_certificate_len;
    unsigned char* serialized_server_certificate = NULL;

    if ((server_certificate_len = i2d_X509(server_certificate, &serialized_server_certificate)) < 0) {
        debug(ERROR, "[E] can not serialize server certificate" << endl);
        return -1;
    }

    /* generate nonce */
    uint32_t nonce;

    if (RAND_poll() != 1) { cerr << "[E] can not initialize PRNG" << endl; return -1; }
    RAND_bytes((unsigned char*)&nonce, sizeof(nonce));

    EVP_PKEY* key = cm->getPrivKey();

    /* sign nonce */
    char server_signature[BUFFER_SIZE];
    int signatureLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_SignInit(ctx, EVP_sha256());
    EVP_SignUpdate(ctx, (unsigned char*)&nonce, sizeof(nonce));
    EVP_SignFinal(ctx, (unsigned char*)server_signature, (unsigned int*)&signatureLen, key);
    EVP_MD_CTX_free(ctx);

    /* prepare M2 */
    M2 m2;
    m2.certLen = htonl(server_certificate_len);
    m2.signLen = htonl(signatureLen);
    m2.nonceS = nonce;

    /* send M2 */
    connection->send((const char*)&m2, sizeof(m2));
    connection->send((const char*)serialized_server_certificate, server_certificate_len);
    connection->send((const char*)server_signature, signatureLen);

    debug(DEBUG, "[D] M2 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m2, sizeof(m2));
    hexdump(DEBUG, (const char *)serialized_server_certificate, server_certificate_len);
    hexdump(DEBUG, (const char *)server_signature, signatureLen);

    /* free memory */
    OPENSSL_free(serialized_server_certificate);

}

void Client::recvCmd() {
    char buffer[BUFFER_SIZE];
//    char ciphertext;
    int recvBytes;
    char shiftRegister[2];

    is.ignore(BUFFER_SIZE);
    is.clear();

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = crypto->recv(connection, buffer + i, 1);
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

bool Client::execute(void) {
    string cmd;

    try {
        /* key exchange handshake */
        handshake();
        this->crypto = new Crypto((const unsigned char*)"0123456789abcdef", (const unsigned char*)"fedcba9876543210", (const unsigned char*)"0000000000000000");

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


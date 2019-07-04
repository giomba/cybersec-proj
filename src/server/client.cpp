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

int Client::handshake(unsigned char*& session_key, unsigned char*& auth_key, unsigned char*& iv) {
    X509* client_certificate = NULL;
    if (receiveM1(client_certificate) == -1) throw ExCertificate("[E] M1");
    if (sendM2(client_certificate, session_key, auth_key, iv) == -1) throw ExCertificate("[E] M2");
    //receiveM3();
    return 0;
}

int Client::receiveM1(X509*& client_certificate) {
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
    client_certificate = d2i_X509(NULL, (const unsigned char**)&serialized_client_certificate, m1.certLen);
    if (!client_certificate){
        debug(WARNING, "[W] cannot deserialize client certificate on socket " << connection->getSocket() << endl);
        goto ripper;
    }

    /* check validity */
    if (cm->verifyCert(client_certificate, "") == -1) {
        debug(WARNING, "[W] client is not authenticated by TrustedCA" << endl);
        goto ripper;
    }
    debug(INFO, "[I] client on socket " << connection->getSocket() << " is authenticated" << endl);

    /* verify nonce signature */
    if (signer->verify(client_certificate, (char*)&(m1.nonceC), sizeof(m1.nonceC), client_signature, m1.signLen) == -1) {
        debug(WARNING, "[W] client's nonce signature is not valid" << endl);
        goto ripper;
    }
    debug(INFO, "[I] valid nonce for client socket " << connection->getSocket() << endl);

    return 0;

    ripper: /* deallocates everything in case of an error */
        delete[] serialized_client_certificate;
        delete[] client_signature;
        return -1;
}

int Client::sendM2(X509* client_certificate, unsigned char*& session_key, unsigned char*& auth_key, unsigned char*& iv) {
    M2 m2;
    EVP_PKEY *client_pubkey = X509_get_pubkey(client_certificate);
	if (!client_pubkey){ debug(ERROR, "cannot extract the pubkey from client certificate" << endl); return -1;}

    /* serialize certificate */
    X509* server_certificate = cm->getCert();
    int server_certificate_len;
    unsigned char* serialized_server_certificate = NULL;

    if ((server_certificate_len = i2d_X509(server_certificate, &serialized_server_certificate)) < 0) {
        debug(ERROR, "[E] can not serialize server certificate" << endl);
        return -1;
    }

    /* generate nonce NB */
    uint32_t nonce;

    if (RAND_poll() != 1) { cerr << "[E] can not initialize PRNG" << endl; return -1; }
    RAND_bytes((unsigned char*)&nonce, sizeof(nonce));

    /* generate Ks, Ka and IV */
    session_key = new unsigned char[AES128_KEY_LEN];
    auth_key = new unsigned char[HMAC_LEN];
    iv = new unsigned char[AES128_KEY_LEN];
    RAND_bytes(session_key, AES128_KEY_LEN);
    RAND_bytes(auth_key, HMAC_LEN);
    RAND_bytes(iv, AES128_KEY_LEN);

    hexdump(DEBUG, (const char*)session_key, AES128_KEY_LEN);
    hexdump(DEBUG, (const char*)auth_key, HMAC_LEN);
    hexdump(DEBUG, (const char*)iv, AES128_KEY_LEN);

    /* asymmetric encrypting - ctx_e = context for encryption */
    /* encrypt session and auth keys with client public key */

    int seal_enc_key_len = EVP_PKEY_size(client_pubkey);
    unsigned char* seal_enc_key = new unsigned char[seal_enc_key_len];
    int seal_iv_len = AES128_BLOCK_LEN;//EVP_CIPHER_iv_length(EVP_aes_128_cbc());
    unsigned char *seal_iv = new unsigned char[seal_iv_len];
    unsigned char* keyblob = new unsigned char[AES128_KEY_LEN + HMAC_LEN + AES128_BLOCK_LEN];
    int update_len, keyblob_len = 0;

    debug(DEBUG, "seal_enc_key_len:\t" << seal_enc_key_len << endl);
    debug(DEBUG, "keyblob_len:\t" << AES128_KEY_LEN + HMAC_LEN + AES128_BLOCK_LEN << endl);

    EVP_CIPHER_CTX *ctx_e = EVP_CIPHER_CTX_new();
    if (EVP_SealInit(ctx_e, EVP_aes_128_cbc(), &seal_enc_key, &seal_enc_key_len, seal_iv, &client_pubkey, 1) == 0){
        debug(ERROR, "[E] EVP_SealInit()" << endl);
        return -1;
    }
    EVP_SealUpdate(ctx_e, keyblob, &update_len, session_key, AES128_KEY_LEN);
    keyblob_len += update_len;
    EVP_SealUpdate(ctx_e, keyblob + keyblob_len, &update_len, auth_key, HMAC_LEN);
    keyblob_len += update_len;

    EVP_SealFinal(ctx_e, keyblob + keyblob_len, &update_len);
    keyblob_len += update_len;
    EVP_CIPHER_CTX_free(ctx_e);

    debug(DEBUG, "seal_enc_key_len:\t" << seal_enc_key_len << endl);
    debug(DEBUG, "keyblob_len:\t" << keyblob_len << endl);

    /* signature - ctx_s = context for signing */
    char server_signature[BUFFER_SIZE];
    int signatureLen;

    EVP_MD_CTX* ctx_s = EVP_MD_CTX_new();
    EVP_SignInit(ctx_s, EVP_sha256());
    EVP_SignUpdate(ctx_s, (unsigned char*)&nonce, sizeof(nonce));
    EVP_SignFinal(ctx_s, (unsigned char*)server_signature, (unsigned int*)&signatureLen, cm->getPrivKey());
    EVP_MD_CTX_free(ctx_s);

    /* prepare M2 */
    m2.certLen = htonl(server_certificate_len);
    m2.signLen = htonl(signatureLen);
    m2.encryptedSymmetricKeyLen = htonl(seal_enc_key_len);
    m2.ivLen = htonl(seal_iv_len);
    m2.keyblobLen = htonl(keyblob_len);
    m2.nonceS = nonce;
    m2.nonceC = htonl(0xcafebabe);  // TODO

    /* send M2 */
    connection->send((const char*)&m2, sizeof(m2));
    connection->send((const char*)serialized_server_certificate, server_certificate_len);
    connection->send((const char*)server_signature, signatureLen);
    connection->send((const char*)seal_enc_key, seal_enc_key_len);
    connection->send((const char*)seal_iv, seal_iv_len);
    connection->send((const char*)keyblob, keyblob_len);
    connection->send((const char*)iv, seal_iv_len);

    debug(DEBUG, "[D] M2 Lengths" << endl);
    debug(DEBUG, "[D] server_certificate_len:\t" << server_certificate_len << endl);
    debug(DEBUG, "[D] signatureLen:\t" << signatureLen << endl);
    debug(DEBUG, "[D] seal_enc_key_len:\t" << seal_enc_key_len << endl);
    debug(DEBUG, "[D] seal_iv_len:\t" << seal_iv_len << endl);
    debug(DEBUG, "[D] keyblob_len:\t" << keyblob_len << endl);

    debug(DEBUG, "[D] M2 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m2, sizeof(m2));
    hexdump(DEBUG, (const char *)serialized_server_certificate, server_certificate_len);
    hexdump(DEBUG, (const char *)server_signature, signatureLen);
    hexdump(DEBUG, (const char *)seal_enc_key, seal_enc_key_len);
    hexdump(DEBUG, (const char *)seal_iv, seal_iv_len);
    hexdump(DEBUG, (const char *)keyblob, keyblob_len);
    hexdump(DEBUG, (const char *)iv, seal_iv_len);

    /* free memory */
    OPENSSL_free(serialized_server_certificate);
    return 0;
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
    unsigned char *session_key;
    unsigned char *auth_key;
    unsigned char *iv;

    try {
        /* key exchange handshake */
        handshake(session_key, auth_key, iv);
        this->crypto = new Crypto((const unsigned char*)session_key, (const unsigned char*)auth_key, (const unsigned char*)iv);
        this->signer = new Signer();

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


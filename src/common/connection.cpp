#include "connection.h"

Connection::Connection(int sd, struct sockaddr_in6 peer, CertManager* cm) {
    debug(INFO, "[I] new connection " << this << endl);
    this->sd = sd;
    this->peer = peer;
    this->cm = cm;
}

Connection::Connection(const char* hostname, uint16_t port, CertManager* cm) {
    // do everything is needed to connect
    if ((this->sd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        throw ExSocket("can not create socket() for new Connection()", errno);
    }
    memset(&peer, 0, sizeof(peer));
    peer.sin6_family = AF_INET6;
    peer.sin6_port = htons(port);
    inet_pton(AF_INET6, hostname, &peer.sin6_addr);

    if (::connect(this->sd, (struct sockaddr*)&peer, sizeof(peer)) == -1) {
        throw ExConnect("can not connect() for new Connection()", errno);
    };

    this->cm = cm;
}

int Connection::handshakeServer() {
    debug(DEBUG, "[D] beginning handshake" << endl);

    /* === --- M1 --- === */
    /* receive M1 */
    M1 m1;
    this->recv((char*)&m1, sizeof(m1));

    /* if client sends an exagerated size for certificate or signature, don't allocate the buffers */
    m1.certLen = ntohl(m1.certLen);
    m1.signLen = ntohl(m1.signLen);
    if (m1.certLen > BUFFER_SIZE) throw ExTooBig("client certificate too big");
    if (m1.signLen > BUFFER_SIZE) throw ExTooBig("nonce signature too big");

    debug(DEBUG, "[D] certLen: " << m1.certLen << "\t" << "signLen: " << m1.signLen << endl);

    unsigned char* serialized_client_certificate = new unsigned char[m1.certLen];
    this->recv((char*)serialized_client_certificate, m1.certLen);
    unsigned char* signature = new unsigned char[m1.signLen];
    this->recv((char*)signature, m1.signLen);

    debug(DEBUG, "[D] received M1 + payload" << endl);
    hexdump(DEBUG, (const char*)&m1, sizeof(m1));
    hexdump(DEBUG, (const char*)serialized_client_certificate, m1.certLen);
    hexdump(DEBUG, (const char*)signature, m1.signLen);

    /* deserialize certificate */
    X509* client_certificate = d2i_X509(NULL, (const unsigned char**)&serialized_client_certificate, m1.certLen);

    /* check validity */
    if (this->cm->verifyCert(client_certificate, "") == -1) {
        debug(ERROR, "[E] client is not authenticated by TrustedCA" << endl);
        throw ExCertificate("client is not authenticated by TrustedCA");
    }
    debug(INFO, "[I] client on socket " << this->sd << " is authenticated" << endl);

    /* verify nonce signature */
    if (cm->verifySignature(client_certificate, (char*)&(m1.nonceC), sizeof(m1.nonceC), signature, m1.signLen) == -1) {
        debug(ERROR, "[E] client's nonce signature is not valid" << endl);
        throw ExCertificate("client nonce signature is not valid");
    }
    debug(INFO, "[I] valid nonce for client socket " << this->sd << endl);

    /* === --- M2 --- === */
    /*
    X509* server_certificate = cm->getCert();
    unsigned char* serialized_server_certificate; */


    X509_free(client_certificate);
}

int Connection::handshakeClient() {
    /* === --- M1 --- === */

    /* serialize certificate */
    X509* client_certificate = cm->getCert();
    int client_certificate_len;
    unsigned char* serialized_client_certificate = NULL;

    if ((client_certificate_len = i2d_X509(client_certificate, &serialized_client_certificate)) < 0) {
        cerr << "[E] can not serialize client certificate" << endl;
        return -1;
    }

    /* generate nonce  */
    uint32_t nonce;

    if (RAND_poll() != 1) { cerr << "[E] can not initialize PRNG" << endl; return -1; }
    RAND_bytes((unsigned char*)&nonce, sizeof(nonce));

    EVP_PKEY* key = cm->getPrivKey();

    /* sign nonce */
    char signature[BUFFER_SIZE];
    int signatureLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_SignInit(ctx, EVP_sha256());
    EVP_SignUpdate(ctx, (unsigned char*)&nonce, sizeof(nonce));
    EVP_SignFinal(ctx, (unsigned char*)signature, (unsigned int*)&signatureLen, key);
    EVP_MD_CTX_free(ctx);


    /* prepare M1 */
    M1 m1;
    m1.certLen = htonl(client_certificate_len);
    m1.signLen = htonl(signatureLen);
    m1.nonceC = nonce;

    /* send M1 */
    this->send((const char*)&m1, sizeof(m1));
    this->send((const char*)serialized_client_certificate, client_certificate_len);
    this->send((const char*)signature, signatureLen);

    debug(DEBUG, "[D] M1 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m1, sizeof(m1));
    hexdump(DEBUG, (const char *)serialized_client_certificate, client_certificate_len);
    hexdump(DEBUG, (const char *)signature, signatureLen);

    /* === --- M2 --- === */
    /* receive M2 */
    /*
    M2 m2;
    this->recv((char*)&m2, sizeof(m2));

    m2.certLen = ntohl(m2.certLen);
    m2.signLen = ntohl(m2.signLen);
    */
    /*
    unsigned char *server_certificate = new unsigned char[m2.certLen];

    // TODO -- I AM HERE



    this->recv((char*)serialized_server_certificate, server_certificate_len);

    free(server_certificate);
    */
    OPENSSL_free(serialized_client_certificate);
    return 0;
}

int Connection::send(const char* buffer, int len) {
    debug(DEBUG, "=== Connection::send()" << endl);
    hexdump(DEBUG, buffer, len);

    int ret = ::send(this->sd, (void*)buffer, len, MSG_NOSIGNAL);
    if (ret <= 0) {
        throw ExSend("can not send()");
    }
    return ret;
}

int Connection::recv(char* buffer, int len) {
    int ret = ::recv(this->sd, (void*)buffer, len, 0);
    if (ret <= 0) {
        throw ExRecv("can not recv()");
    }
    return ret;
}

Connection::~Connection() {
    debug(INFO, "[I][" << this->sd << "] destroyng connection " << this << endl);
    if (this->sd != 0) close(this->sd);
    // close connections and destroy created sockets, if necessary
    // (eg. if socket was created using this->connect() )
}


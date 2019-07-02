#include "protocol.h"

regex parola = regex("[A-Za-z0-9\\._]*");

int handshakeServer(Connection* connection) {
    debug(DEBUG, "[D] beginning handshake" << endl);

    /* === --- M1 --- === */
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
    unsigned char* signature = new unsigned char[m1.signLen];
    connection->recv((char*)signature, m1.signLen);

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

int handshakeClient(Connection* connection) {
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
    connection->send((const char*)&m1, sizeof(m1));
    connection->send((const char*)serialized_client_certificate, client_certificate_len);
    connection->send((const char*)signature, signatureLen);

    debug(DEBUG, "[D] M1 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m1, sizeof(m1));
    hexdump(DEBUG, (const char *)serialized_client_certificate, client_certificate_len);
    hexdump(DEBUG, (const char *)signature, signatureLen);

    /* === --- M2 --- === */
    /* receive M2 */
    /*
    M2 m2;
    connection->recv((char*)&m2, sizeof(m2));

    m2.certLen = ntohl(m2.certLen);
    m2.signLen = ntohl(m2.signLen);
    */
    /*
    unsigned char *server_certificate = new unsigned char[m2.certLen];

    // TODO -- I AM HERE



    connection->recv((char*)serialized_server_certificate, server_certificate_len);

    free(server_certificate);
    */
    OPENSSL_free(serialized_client_certificate);
    return 0;
}


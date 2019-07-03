#include "protocol.h"

regex parola = regex("[A-Za-z0-9\\._]*");

int handshakeClient(Connection* connection, CertManager* cm) {
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
    char client_signature[BUFFER_SIZE];
    int signatureLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_SignInit(ctx, EVP_sha256());
    EVP_SignUpdate(ctx, (unsigned char*)&nonce, sizeof(nonce));
    EVP_SignFinal(ctx, (unsigned char*)client_signature, (unsigned int*)&signatureLen, key);
    EVP_MD_CTX_free(ctx);

    /* prepare M1 */
    M1 m1;
    m1.certLen = htonl(client_certificate_len);
    m1.signLen = htonl(signatureLen);
    m1.nonceC = nonce;

    /* send M1 */
    connection->send((const char*)&m1, sizeof(m1));
    connection->send((const char*)serialized_client_certificate, client_certificate_len);
    connection->send((const char*)client_signature, signatureLen);

    debug(DEBUG, "[D] M1 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m1, sizeof(m1));
    hexdump(DEBUG, (const char *)serialized_client_certificate, client_certificate_len);
    hexdump(DEBUG, (const char *)client_signature, signatureLen);

    /* free memory */
    OPENSSL_free(serialized_client_certificate);

    /* === --- M2 --- === */
    /* receive M2 */
    M2 m2;
    connection->recv((char*)&m2, sizeof(m2));

    /* if client sends an exagerated size for certificate or signature, don't allocate the buffers */
    m2.certLen = ntohl(m2.certLen);
    m2.signLen = ntohl(m2.signLen);
    if (m2.certLen > BUFFER_SIZE) throw ExTooBig("client certificate too big");
    if (m2.signLen > BUFFER_SIZE) throw ExTooBig("nonce signature too big");

    debug(DEBUG, "[D] certLen: " << m2.certLen << "\t" << "signLen: " << m2.signLen << endl);

    unsigned char* serialized_server_certificate = new unsigned char[m2.certLen];
    connection->recv((char*)serialized_server_certificate, m2.certLen);
    unsigned char* server_signature = new unsigned char[m2.signLen];
    connection->recv((char*)server_signature, m2.signLen);

    debug(DEBUG, "[D] received M1 + payload" << endl);
    hexdump(DEBUG, (const char*)&m2, sizeof(m2));
    hexdump(DEBUG, (const char*)serialized_server_certificate, m2.certLen);
    hexdump(DEBUG, (const char*)server_signature, m2.signLen);

    /* deserialize certificate */
    X509* server_certificate = d2i_X509(NULL, (const unsigned char**)&serialized_server_certificate, m2.certLen);
    if (!server_certificate){
        debug(ERROR, "[E] cannot deserialize server certificate" << endl);
        return -1;
    }

    /* check validity */
    if (cm->verifyCert(server_certificate, "server") == -1) {
        debug(ERROR, "[E] server is not authenticated by TrustedCA" << endl);
        throw ExCertificate("server is not authenticated by TrustedCA");
    }
    debug(INFO, "[I] server is authenticated" << endl);

    /* verify nonce signature */
    if (cm->verifySignature(server_certificate, (char*)&(m2.nonceS), sizeof(m2.nonceS), server_signature, m2.signLen) == -1) {
        debug(ERROR, "[E] server's nonce signature is not valid" << endl);
        throw ExCertificate("server nonce signature is not valid");
    }
    debug(INFO, "[I] valid nonce for server"<< endl);

    /* free memory */
    //delete[] server_signature;
    //delete[] serialized_server_certificate;

    return 0;
}


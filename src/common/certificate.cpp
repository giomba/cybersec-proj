#include "certificate.h"

Certificate::Certificate(void) {
    this->cert = NULL;
}

void Certificate::fromX509(X509* cert) {
    this->cert = cert;
}

void Certificate::fromString(string buffer) {
    const char* tmpstr = buffer.c_str();
    this->cert = d2i_X509(NULL, (const unsigned char**)&tmpstr, buffer.size());
    if (this->cert == NULL) {
        debug(WARNING, "[W] cannot deserialize certificate" << endl);
        openssl_perror();
        throw ExCertificate("cannot deserialize certificate");
    }
}

Certificate::~Certificate(void) {
    if (this->cert) X509_free(this->cert);
}

X509* Certificate::getX509(void) {
    assert(this->cert != NULL);
    return this->cert;
}

string Certificate::str() {
    assert(this->cert != NULL);
    int len;
    unsigned char* serialized_certificate = NULL;

    len = i2d_X509(this->cert, &serialized_certificate);
    if (len < 0) {
        debug(WARNING, "[W] cannot serialize certificate" << endl);
        throw ExCertificate("Certificate::str(): cannot serialize certificate");
    }

    string ret = string((const char*)serialized_certificate, len);

    OPENSSL_free(serialized_certificate);

    return ret;
}

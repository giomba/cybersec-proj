#include "certificate.h"

Certificate::Certificate(void) {
    this->cert = NULL;
}

Certificate::Certificate(X509* cert) {
    this->cert = cert;
}

Certificate::Certificate(string buffer) {
    const char* tmpstr = buffer.c_str();
    this->cert = d2i_X509(NULL, (const unsigned char**)&tmpstr, buffer.size());
    if (this->cert == NULL) {
        debug(ERROR, "[E] cannot deserialize certificate" << endl);
        openssl_perror();
        throw ExCertificate("cannot deserialize certificate");
    }
}

Certificate::~Certificate(void) {
    X509_free(this->cert);
}

X509* Certificate::getX509(void) {
    return this->cert;
}

string Certificate::str() {
    int len;
    unsigned char* serialized_certificate = NULL;

    if ((len = i2d_X509(this->cert, &serialized_certificate)) < 0) {
        debug(ERROR, "[E] cannot serialize certificate" << endl);
        openssl_perror();
        throw ExCertificate("cannot serialize certificate");
    }

    vhexdump(DEBUG, (const char*)serialized_certificate, len);

    string ret = string((const char*)serialized_certificate, len);

    OPENSSL_free(serialized_certificate);

    return ret;
}
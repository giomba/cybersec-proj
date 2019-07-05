#include "certificate.h"

Certificate::Certificate(void) {
    this->cert = NULL;
}

Certificate::Certificate(X509* cert) {
    this->cert = cert;
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
        throw ExCertificate("[E] cannot serialize certificate");
    }

    return string((const char*)serialized_certificate, len);
}
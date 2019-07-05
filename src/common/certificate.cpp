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
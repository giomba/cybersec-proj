#include "rsakey.h"

const string KEY_PATH = "cert/";

RSAKey::RSAKey() {
    this->key = NULL;
}

RSAKey::~RSAKey() {
    if (this->key != NULL) EVP_PKEY_free(this->key);
}

RSAKey::RSAKey(const RSAKey& old) {
    assert(false);
}
RSAKey& RSAKey::operator=(const RSAKey& other) {
    assert(false);
}

void RSAKey::fromUserName(string username) {
    FILE *file = fopen((KEY_PATH + username + "_key.pem").c_str(), "r");
    if (!file) throw ExCertificate("cannot open private key");
	this->key = PEM_read_PrivateKey(file, NULL, NULL, NULL);
	fclose(file);
	if (! this->key) throw ExCertificate("can not read my private key");
}

void RSAKey::fromCertificate(Certificate& certificate) {
    this->key = X509_get_pubkey(certificate.getX509());
}

void RSAKey::fromString(string buffer) {
    // TODO -- init me
}

EVP_PKEY* RSAKey::getPKEY(void) {   // TODO DEBUG ME
    assert(this->key != NULL);
    return this->key;
}

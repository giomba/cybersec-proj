#include "rsakey.h"

const string KEY_PATH = "cert/";

RSAKey::RSAKey() {
    this->key = NULL;
}

RSAKey::~RSAKey() {
    if (this->key != NULL) EVP_PKEY_free(this->key);
}

void RSAKey::fromUserName(string username) {
    FILE *file = fopen((KEY_PATH + username + "_key.pem").c_str(), "r");
    if (!file) throw ExCertificate("RSAKey::fromUserName(): cannot open private key");
	this->key = PEM_read_PrivateKey(file, NULL, NULL, NULL);
	fclose(file);
	if (! this->key) throw ExCertificate("RSAKey::fromUserName(): can not read my private key");
}

void RSAKey::fromCertificate(Certificate& certificate) {
    this->key = X509_get_pubkey(certificate.getX509());
    if (!this->key) throw ExCertificate("RSAKey::fromCertificate(): cannot read public key");
}

EVP_PKEY* RSAKey::getPKEY(void) {
    assert(this->key != NULL);
    return this->key;
}

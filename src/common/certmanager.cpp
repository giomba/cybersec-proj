#include "certmanager.h"

CertManager::CertManager(string cert_name){
	// read CA certificate
	X509* CA_cert;
	FILE* file = fopen((CERT_PATH + "TrustedCA_cert.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open CA_cert.pem" << endl); throw ExCertificate(); }
    CA_cert = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);
    if (!CA_cert) { debug(FATAL, "cannot read CA certificate" << endl); throw ExCertificate(); }

	// read Certificate Revocation List
	X509_CRL* crl;
    file = fopen((CERT_PATH + "TrustedCA_crl.pem").c_str(), "r");
    if (!file) { debug(ERROR, "cannot open CA_crl.pem" << endl); throw ExCertificate(); }
    crl = PEM_read_X509_CRL(file, NULL, NULL, NULL);
	fclose(file);
    if (!crl) { debug(ERROR, "cannot read CRL" << endl); throw ExCertificate(); }

	// build store
    this->store = X509_STORE_new();
	if (!this->store) { debug(FATAL, "cannot create the CA store" << endl); throw ExCertificate(); }
    if (X509_STORE_add_cert(this->store, CA_cert) != 1) { debug(FATAL, "cannot add CA_cert to store" << endl); throw ExCertificate(); }
    if (X509_STORE_add_crl(this->store, crl) != 1) { debug(FATAL, "cannot add CRL to store" << endl); throw ExCertificate(); }
    X509_STORE_set_flags(this->store, X509_V_FLAG_CRL_CHECK);

	debug(INFO, "CA store created successfully" << endl);

	// read my certificate
	file = fopen((CERT_PATH + cert_name + "_cert.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open " + cert_name + ".pem" << endl); throw ExCertificate(); }
    this->cert = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);
    if (!this->cert) { debug(FATAL, "can not read my certificate" << endl); throw ExCertificate(); } 

	// load my private key
    file = fopen((CERT_PATH + cert_name + "_key.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open " + cert_name + " private key" << endl); throw ExCertificate(); }
	this->privkey = PEM_read_PrivateKey(file, NULL, NULL, NULL);
	fclose(file);
	if (!this->privkey) { debug(FATAL, "cannot read my private key" << endl); throw ExCertificate(); } 
}

CertManager::~CertManager(){
	X509_STORE_free(this->store);
	EVP_PKEY_free(this->privkey);
	X509_free(this->cert);
	debug(INFO, "[I] destroying CA store and private key" << endl);
}

EVP_PKEY* CertManager::getPrivKey(){
	return this->privkey;
}

X509* CertManager::getCert(){
	return this->cert;
}

int CertManager::verifyCert(X509* cert, string name){
	// verification
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
	if (!ctx) { debug(ERROR, "cannot create ctx on verifying" << endl); return -1; }
    if (X509_STORE_CTX_init(ctx, this->store, cert, NULL) != 1) { 
		X509_STORE_CTX_free(ctx); 
		debug(ERROR, "cannot init ctx on verifying" << endl); 
		return -1; 
	}
    if (X509_verify_cert(ctx) != 1) { 
		X509_STORE_CTX_free(ctx); 
		debug(ERROR, "cert verification failed" << endl); 
		return -1; 
	}

	if (!name.empty()){
		// check subject name of the server
		X509_NAME* subject_name = X509_get_subject_name(cert);
		string str(X509_NAME_oneline(subject_name, NULL, 0));
		free(subject_name);
		debug(INFO, "cert belongs to " + str << endl);
		if ((int)str.find("CN=server") == -1) { debug(FATAL, "server name does not match" << endl); return -1; }
	}

	debug(INFO, "cert verification succeded" << endl);
	return 0;
}

int CertManager::verifySignature(X509* cert, char* msg, int msg_len, unsigned char* signature, int signature_len){
	/* get public key from certificate */
    EVP_PKEY* pubkey = X509_get_pubkey(cert);
	if (!pubkey){ debug(ERROR, "cannot extract the pubkey from certificate" << endl); return -1;}

    /* check signature */
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_VerifyInit(ctx, EVP_sha256());
    EVP_VerifyUpdate(ctx, msg, msg_len);
    int ret = EVP_VerifyFinal(ctx, signature, signature_len, pubkey);
    EVP_MD_CTX_free(ctx);

	return (ret != 1) ? -1 : 0;
}
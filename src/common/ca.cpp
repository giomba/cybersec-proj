#include "ca.h"

CertificationAuthority::CertificationAuthority(string cert_name){
	// read CA certificate
	X509* CA_cert;
	FILE* file = fopen((CERT_PATH + "TrustedCA_cert.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open CA_cert.pem" << endl); exit(-1); }
    CA_cert = PEM_read_X509(file, NULL, NULL, NULL);
    if (!cert) { debug(FATAL, "can not read CA certificate" << endl); exit(-1); }
    fclose(file);

	// read Certificate Revocation List
	X509_CRL* crl;
    file = fopen((CERT_PATH + "TrustedCA_crl.pem").c_str(), "r");
    if (!file) { debug(ERROR, "cannot open CA_crl.pem" << endl); exit(-1); }
    crl = PEM_read_X509_CRL(file, NULL, NULL, NULL);
    if (!crl) { debug(ERROR, "cannot read CRL" << endl); exit(-1); }
    fclose(file);

	// build store
    this->store = X509_STORE_new();
	if (!this->store) { debug(FATAL, "cannot create the CA store" << endl); exit(-1); }
    if (X509_STORE_add_cert(this->store, CA_cert) != 1) { debug(FATAL, "cannot add CA_cert to store" << endl); exit(-1); }
    if (X509_STORE_add_crl(this->store, crl) != 1) { debug(FATAL, "cannot add CRL to store" << endl); exit(-1); }
    X509_STORE_set_flags(this->store, X509_V_FLAG_CRL_CHECK);

	debug(INFO, "CA store created successfully" << endl);

	// read my certificate
	file = fopen((CERT_PATH + cert_name + "_cert.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open " + cert_name + ".pem" << endl); exit(-1); }
    this->cert = PEM_read_X509(file, NULL, NULL, NULL);
    if (!this->cert) { debug(FATAL, "can not read my certificate" << endl); exit(-1); }
    fclose(file);
}

CertificationAuthority::~CertificationAuthority(){
	X509_STORE_free(this->store);
	debug(INFO, "[I] destroying CA store" << endl);
}

X509* CertificationAuthority::getCert(){
	return this->cert;
}

int CertificationAuthority::cert_verification(X509* cert, string name){
	// verification
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
	if (!ctx) { debug(ERROR, "cannot create ctx on verifying" << endl); return -1; }
    if (X509_STORE_CTX_init(ctx, this->store, cert, NULL) != 1) { debug(ERROR, "cannot init ctx on verifying" << endl); return -1; }
    if (X509_verify_cert(ctx) != 1) { debug(ERROR, "cert verification failed" << endl); return -1; }
    X509_STORE_CTX_free(ctx);

	if (!name.empty()){
		// check subject name
		X509_NAME* subject_name = X509_get_subject_name(cert);
		string str(X509_NAME_oneline(subject_name, NULL, 0));
		debug(INFO, "cert belongs to " + str << endl);
		if (str != name) { debug(FATAL, "server name does not match" << endl); return -1; }
		free(subject_name);
	}

	debug(INFO, "cert verification succeded" << endl);
	return 0;
}

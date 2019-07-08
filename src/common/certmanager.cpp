#include "certmanager.h"

const string CERT_PATH = "cert/";

CertManager::CertManager(string username, vector<string>& authPeersList) : authPeersList(authPeersList) {
	// read CA certificate
	X509* CA_cert;
	FILE* file = fopen((CERT_PATH + "TrustedCA_cert.pem").c_str(), "r");
    if (!file) throw ExCertificate("CertManager::CertManager(): cannot open TrustedCA_cert.open");
    CA_cert = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);
    if (!CA_cert) throw ExCertificate("CertManager::CertManager(): cannot read CA_cert");

	// read Certificate Revocation List
	X509_CRL* crl;
    file = fopen((CERT_PATH + "TrustedCA_crl.pem").c_str(), "r");
    if (!file) throw ExCertificate("CertManager::CertManager(): cannot open TrustedCA_crl.pem");
    crl = PEM_read_X509_CRL(file, NULL, NULL, NULL);
	fclose(file);
    if (!crl) throw ExCertificate("CertManager::CertManager(): cannot read crl");

	// build store
    this->store = X509_STORE_new();
	if (!this->store) throw ExCertificate("CertManager::CertManager(): cannot create the store");
    if (X509_STORE_add_cert(this->store, CA_cert) != 1) throw ExCertificate("CertManager::CertManager(): cannot add CA_cert to store");
    if (X509_STORE_add_crl(this->store, crl) != 1) throw ExCertificate("CertManager::CertManager(): cannot add crl to store");
    X509_STORE_set_flags(this->store, X509_V_FLAG_CRL_CHECK);

	debug(INFO, "[I] CA store created successfully" << endl);

	// read my certificate
	X509* my_cert;
	file = fopen((CERT_PATH + username + "_cert.pem").c_str(), "r");
    if (!file) throw ExCertificate("can not open PEM file: certificate");
    my_cert = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);
    if (my_cert == NULL) throw ExCertificate("can not parse PEM certificate");
	this->cert.fromX509(my_cert);

	this->privkey.fromUserName(username);

	X509_free(CA_cert);
	X509_CRL_free(crl);
}

CertManager::~CertManager(){
	X509_STORE_free(this->store);
	debug(DEBUG, "[D] destroying Cert Manager" << endl);
}

Certificate& CertManager::getCert(){
	return this->cert;
}

RSAKey& CertManager::getPrivKey() {
	return this->privkey;
}

//Here we are also doing the client verification
void CertManager::verifyCert(Certificate& cert) {
	// verification
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
	if (!ctx) throw ExCertificate("CertManager::verify(): X509_STORE_CTX_new()");

	bool cert_pass = (X509_STORE_CTX_init(ctx, this->store, cert.getX509(), NULL) == 1)
					 && (X509_verify_cert(ctx) == 1);

	X509_STORE_CTX_free(ctx);

	if (!cert_pass) throw ExCertificate("CertManager::verify(): invalid certificate");

	bool name_pass = false;
	if (!authPeersList.empty()) {
		// get subject name
		X509_NAME* subject_name = X509_get_subject_name(cert.getX509());
		char* oneline = X509_NAME_oneline(subject_name, NULL, 0);
		string subject_name_str = string(oneline);
		free(oneline);

		debug(DEBUG, "[D] cert belongs to " + subject_name_str << endl);

		//check if the name is in the list
		unsigned int i = 0;
		for(i = 0; i < authPeersList.size() && ((int)subject_name_str.find("CN=" + authPeersList[i]) == -1); i++);
		if (i == authPeersList.size()) { debug(DEBUG, "[D] user not in the list" << endl); }
		else {
			name_pass = true;
			debug(DEBUG, "[D] " << authPeersList[i] << " is in the list" << endl);
		}
	}

	if (!name_pass) throw ExCertificate("CertManager::verify(): unauthorized client");
	debug(DEBUG, "[D] peer's certificate valid" << endl);

	return;
}

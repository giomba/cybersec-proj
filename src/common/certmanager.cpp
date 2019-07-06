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

	//intializing the list of clients
    clientList.push_back("barba");
    clientList.push_back("alice");
    clientList.push_back("tommy");

	// read my certificate
	X509* my_cert;
	file = fopen((CERT_PATH + cert_name + "_cert.pem").c_str(), "r");
    if (!file) { debug(FATAL, "cannot open " + cert_name + ".pem" << endl); throw ExCertificate(); }
    my_cert = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);
    if (my_cert == NULL) { debug(FATAL, "can not read my certificate" << endl); throw ExCertificate(); }

	this->cert = new Certificate(my_cert);

	X509_free(CA_cert);
	X509_CRL_free(crl);
}

CertManager::~CertManager(){
	X509_STORE_free(this->store);
	delete this->cert;
	debug(INFO, "[I] destroying CA store and certificate" << endl);
}

Certificate* CertManager::getCert(){
	return this->cert;
}

//Here we are also doing the client verification
void CertManager::verifyCert(Certificate* cert) {
	// verification
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
	if (!ctx) throw ExCertificate("CertManager::verify(): X509_STORE_CTX_new()");

	bool cert_pass = (X509_STORE_CTX_init(ctx, this->store, cert->getX509(), NULL) == 1)
					 && (X509_verify_cert(ctx) == 1);

	X509_STORE_CTX_free(ctx);

	bool name_pass = false;
	if (!clientList.empty()) {
		// get subject name
		X509_NAME* subject_name = X509_get_subject_name(cert->getX509());
		string subject_name_str = string(X509_NAME_oneline(subject_name, NULL, 0));
		
		debug(INFO, "[I] cert belongs to " + subject_name_str << endl);
		
		//check if the name is in the list
		debug(DEBUG, "[D] client list size: " << clientList.size() << endl);
		unsigned int i = 0;
		for(i = 0; i < clientList.size() && ((int)subject_name_str.find("CN=" + clientList[i]) == -1); i++);
		debug(DEBUG, "[D] " << clientList[i] << " is in the list" << endl);
		name_pass = (i == clientList.size()) ? false : true;
		free(subject_name);
	}

	if (!cert_pass) throw ExCertificate("CertManager::verify(): invalid certificate");
	if (!name_pass) throw ExCertificate("CertManager::verify(): unauthorized client");
	debug(INFO, "[I] client verified successfully" << endl);

	return;
}

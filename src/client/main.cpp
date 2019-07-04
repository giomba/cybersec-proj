#include "client.h"

using namespace std;

/****************************************/
/*              HANDSHAKE               */
/****************************************/

int handshake(){
    return (sendM1() != -1) & (receiveM2() != -1) & (sendM3() != -1);
}

int sendM1(){
    /* serialize certificate */
    X509* certificate = cm->getCert();
    int certificate_len;
    unsigned char* serialized_certificate = NULL;

    if ((certificate_len = i2d_X509(certificate, &serialized_certificate)) < 0) {
        cerr << "[E] can not serialize client certificate" << endl;
        return -1;
    }

    /* generate nonce  */
    uint32_t nonce;

    if (RAND_poll() != 1) { cerr << "[E] can not initialize PRNG" << endl; return -1; }
    RAND_bytes((unsigned char*)&nonce, sizeof(nonce));

    EVP_PKEY* key = cm->getPrivKey();

    /* sign nonce */
    char signature[BUFFER_SIZE];
    int signatureLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_SignInit(ctx, EVP_sha256());
    EVP_SignUpdate(ctx, (unsigned char*)&nonce, sizeof(nonce));
    EVP_SignFinal(ctx, (unsigned char*)signature, (unsigned int*)&signatureLen, key);
    EVP_MD_CTX_free(ctx);

    /* prepare M1 */
    M1 m1;
    m1.certLen = htonl(certificate_len);
    m1.signLen = htonl(signatureLen);
    m1.nonceC = nonce;

    /* send M1 */
    connection->send((const char*)&m1, sizeof(m1));
    connection->send((const char*)serialized_certificate, certificate_len);
    connection->send((const char*)signature, signatureLen);

    debug(DEBUG, "[D] M1 + Payload" << endl);
    hexdump(DEBUG, (const char *)&m1, sizeof(m1));
    hexdump(DEBUG, (const char *)serialized_certificate, certificate_len);
    hexdump(DEBUG, (const char *)signature, signatureLen);

    /* free memory */
    OPENSSL_free(serialized_certificate);
    return 0;
}

int receiveM2(){
    M2 m2;
    connection->recv((char*)&m2, sizeof(m2));

    /* if server sends an exagerated size for certificate or signature, don't allocate the buffers */
    m2.certLen = ntohl(m2.certLen);
    m2.signLen = ntohl(m2.signLen);
    m2.encryptedSymmetricKeyLen = ntohl(m2.encryptedSymmetricKeyLen);
    m2.ivLen = ntohl(m2.ivLen);
    m2.keyblobLen = ntohl(m2.keyblobLen);

    // TODO -- check too big things!!!
    if (m2.certLen > BUFFER_SIZE) throw ExTooBig("client certificate too big");
    if (m2.signLen > BUFFER_SIZE) throw ExTooBig("nonce signature too big");

    //debug(DEBUG, "[D] certLen: " << m2.certLen << "\t" << "signLen: " << m2.signLen << endl);
    debug(DEBUG, "[D] received M2" << endl);
    hexdump(DEBUG, (const char*)&m2, sizeof(m2));

    unsigned char* serialized_certificate = new unsigned char[m2.certLen];
    connection->recv((char*)serialized_certificate, m2.certLen);
    unsigned char* signature = new unsigned char[m2.signLen];
    connection->recv((char*)signature, m2.signLen);
    unsigned char* seal_enc_key = new unsigned char[m2.encryptedSymmetricKeyLen];
    connection->recv((char*)seal_enc_key, m2.encryptedSymmetricKeyLen);
    unsigned char* seal_iv = new unsigned char[m2.ivLen];
    connection->recv((char*)seal_iv, m2.ivLen);
    unsigned char* keyblob = new unsigned char[m2.keyblobLen];
    connection->recv((char*)keyblob, m2.keyblobLen);
    iv = new unsigned char[m2.ivLen];
    connection->recv((char*)iv, m2.ivLen);

    debug(DEBUG, "[D] received M2 payload" << endl);
    hexdump(DEBUG, (const char*)serialized_certificate, m2.certLen);
    hexdump(DEBUG, (const char*)signature, m2.signLen);
    hexdump(DEBUG, (const char*)seal_enc_key, m2.encryptedSymmetricKeyLen);
    hexdump(DEBUG, (const char*)seal_iv, m2.ivLen);
    hexdump(DEBUG, (const char*)keyblob, m2.keyblobLen);
    hexdump(DEBUG, (const char*)iv, m2.ivLen);
    

    /* deserialize certificate */
    X509* server_certificate = d2i_X509(NULL, (const unsigned char**)&serialized_certificate, m2.certLen);
    if (!server_certificate){
        debug(ERROR, "[E] cannot deserialize server certificate" << endl);
        return -1;
    }

    /* check validity */
    if (cm->verifyCert(server_certificate, "server") == -1) {
        debug(ERROR, "[E] server is not authenticated by TrustedCA" << endl);
        throw ExCertificate("server is not authenticated by TrustedCA");
    }
    debug(INFO, "[I] server is authenticated" << endl);

    /* verify nonce signature */
    if (cm->verifySignature(server_certificate, (char*)&(m2.nonceS), sizeof(m2.nonceS), signature, m2.signLen) == -1) {
        debug(ERROR, "[E] server's nonce signature is not valid" << endl);
        throw ExCertificate("server nonce signature is not valid");
    }
    debug(INFO, "[I] valid nonce received from the server"<< endl);

    /* TODO -- free memory */ 
    //delete[] server_signature;
    //delete[] serialized_server_certificate;
    
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	int ret = EVP_OpenInit(ctx, EVP_aes_128_cbc(), seal_enc_key, m2.encryptedSymmetricKeyLen, seal_iv, cm->getPrivKey());
    if(ret == 0){
		debug(ERROR, "[E] can't openinit the seal"<<endl);
		}
	unsigned char* sharedKeys = new unsigned char[m2.keyblobLen];
	int outLen;
	EVP_OpenUpdate(ctx, sharedKeys, &outLen, keyblob, m2.keyblobLen);
	int sharedKeyLen = outLen;
	ret = EVP_OpenFinal(ctx, sharedKeys + sharedKeyLen, &outLen);
	if(ret == 0){
		debug(ERROR, "[E] open final in the client for M2 not working");
		}
	sharedKeyLen += outLen;
	EVP_CIPHER_CTX_free(ctx);
	sessionKey = new unsigned char[AES128_KEY_LEN];
	
	authKey = new unsigned char[EVP_MD_size(EVP_sha256())];
	memcpy(sessionKey, sharedKeys, AES128_KEY_LEN);
	memcpy(authKey, sharedKeys + AES128_KEY_LEN, EVP_MD_size(EVP_sha256()));
	
	hexdump(DEBUG, (const char*)sessionKey, AES128_KEY_LEN);
	hexdump(DEBUG, (const char*)authKey, EVP_MD_size(EVP_sha256()));
    return 0;
}

int sendM3(){
    /* TODO */
    return 0;
}

/****************************************/
/*              SEND/RECV               */
/****************************************/

void send_cmd(string cmd){
    int size = strlen(cmd.c_str());
    //char *ciphertext = new char[size];

//    crypto->encrypt(ciphertext, cmd.c_str(), size);
//    connection->send(ciphertext, size);

    crypto->send(connection, cmd.c_str(), size);

    //delete(ciphertext);
}

void send_fragment(const char *buffer, const int len){
//    char *ciphertext = new char[len];

//    crypto->encrypt(ciphertext, buffer, len);
//    connection->send(ciphertext, len);
    crypto->send(connection, buffer, len);

    //delete(ciphertext);
}

void send_file(string filepath, string filename, int64_t size){
    char buffer[BUFFER_SIZE];
    ifstream file;
    int fragment;
    int64_t remainingBytes = size;

    file.open(filepath, ios::in | ios::binary);
    if (file.is_open()){
        while( remainingBytes > 0 ){
            fragment = (remainingBytes > BUFFER_SIZE) ? BUFFER_SIZE : remainingBytes;
            file.read(buffer, fragment);
            send_fragment(buffer, fragment);
            remainingBytes -= fragment;
            show_progress((double)(size-remainingBytes)/size);
        }
        cout << "File '" << filename << "' stored successfully" << endl;
        file.close();
    } else {
        cerr << error << endl;
    }
}

void recv_response(){
    char buffer[BUFFER_SIZE];
//    char d_buffer[BUFFER_SIZE];

    int recvBytes;
    char shiftRegister[2];

    memset(buffer, 0, BUFFER_SIZE);
//    memset(d_buffer, 0, BUFFER_SIZE);
    is.clear();
    is.str("");

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = crypto->recv(connection, buffer + i, 1); //connection->recv(buffer + i, 1);
//        crypto->decrypt(d_buffer + i, buffer + i, 1);

        if (recvBytes == 1) {
            shiftRegister[0] = shiftRegister[1];
            shiftRegister[1] = buffer[i];

            // if all command and parameters have been received...
            if (shiftRegister[0] == '\n' && shiftRegister[1] == '\n') {
                buffer[i + 1] = '\0';
                break;
            }
        }
    }
    buffer[BUFFER_SIZE - 1] = '\0';

    is.str(string(buffer));
}

int recv_fragment(char* buffer, const int len){
    int recvBytes;
//    char *ciphertext = new char[len];

/*    recvBytes = connection->recv(ciphertext, len);
    crypto->decrypt(buffer, ciphertext, len);*/

    recvBytes = crypto->recv(connection, buffer, len);

//    delete(ciphertext);

    return recvBytes;
}

void recv_list(){
    char buffer[BUFFER_SIZE];
    int response;
    string tag;
    int64_t msg_len;
    int64_t recvBytes = 0;
    int fragment;

    is >> response;
    if (response == OK){
        is >> tag;

        if (tag != "Size:"){
            cerr << error << endl;
            return;
        }

        is >> msg_len;

        if (msg_len < 0){
            cerr << error << endl;
            return;
        }

        is.clear();
        is.str("");
        memset(buffer, 0, BUFFER_SIZE);

        while(recvBytes < msg_len){
            fragment = ((msg_len - recvBytes) > BUFFER_SIZE) ? BUFFER_SIZE : (msg_len - recvBytes);
            recvBytes += recv_fragment(buffer, fragment);
            is.str(string(buffer));
            string file;
            while(is >> file){
                cout << file << endl;
            }
        }
    } else {
        cerr << error << endl;
    }
}

void recv_file(string filename){
    char buffer[BUFFER_SIZE];
    int response;
    string tag;
    int64_t filesize;
    int64_t recvBytes = 0;
    int64_t currBytes;
    int fragment;

    is >> response;
    if (response == OK){
        is >> tag;

        if (tag != "Size:"){
            cerr << error << endl;
            return;
        }

        is >> filesize;

        if (filesize < 0){
            cerr << error << endl;
            return;
        }

        is.clear();
        is.str("");
        memset(buffer, 0, BUFFER_SIZE);


        ofstream file;
        file.open(CLIENT_ROOT + filename, ios::out | ios::binary);
        if (file.is_open()){
            while(recvBytes < filesize){
                fragment = ((filesize - recvBytes) > BUFFER_SIZE) ? BUFFER_SIZE : (filesize - recvBytes);
                currBytes = recv_fragment(buffer, fragment);
                file.write(buffer, currBytes);
                recvBytes += currBytes;
                show_progress((double)recvBytes/filesize);
            }
            file.close();
            cout << "File '" << filename << "' saved successfully in " << CLIENT_ROOT << endl;
        } else {
            cout << "error: dir '" << CLIENT_ROOT << "' does not exist" << endl;
        }
    } else if (response == BAD_FILE) {
        cout << filename << ": No such file" << endl;
    } else
        cerr << error << endl;
}

/********************************/
/*            UTILS             */
/********************************/

CommandType str2cmd(string str){
    if (str == "help")   { return HELP; }
    if (str == "lls")    { return L_LIST; }
    if (str == "rls")    { return R_LIST; }
    if (str == "q")      { return QUIT; }
    if (str == "get")    { return RETR; }
    if (str == "put")    { return STOR; }
    if (str == "rm")     { return DELE; }
    return BAD_REQ;
}

void show_progress(double ratio){
    ostringstream os;
    cout << "\b\b\b\b\b\b\b";

    int perc = ratio * 100.0;
    os << perc << "%";
    if (perc >= 100)
        os << "   " <<  endl;
    else {
	os << " [";
	switch(perc % 4){
	    case 0: os << "|"; break;
	    case 1: os << "/"; break;
	    case 2: os << "-"; break;
	    case 3: os << "\\"; break;
	}
	os << "]";
    }
    cout << os.str() << flush;
}

void parse_cmd(){
    string str_cmd, arg;
    is >> str_cmd >> arg;

    CommandType cmd = str2cmd(str_cmd);

    switch(cmd){
        case HELP:   cmd_help(); break;
        case QUIT:   cmd_quit(); break;
        case L_LIST: cmd_local_list(arg); break;
        case R_LIST: cmd_remote_list(); break;
        case RETR:   cmd_retr(arg); break;
        case STOR:   cmd_stor(arg); break;
        case DELE:   cmd_dele(arg); break;
        default:     cmd_unknown(str_cmd);
    }
}

bool is_file(string file){
    struct stat stats;
    stat(file.c_str(), &stats);
    return S_ISREG(stats.st_mode);
}

bool check_and_get_file_size(string filename, int64_t &size){
    if (!is_file(filename)){
        cout << filename << ": No such file" << endl;
        return false;
    }

    ifstream file;
    file.open(filename, ios::in | ios::binary | ios::ate);

    if (file.is_open()){
        // get the size because the cursor is at the end by means of ios::ate
        size = file.tellg();

        if (size > MAX_FILE_SIZE){
            cout << filename << ": exceeded maximum dimension (4 GB)" << endl;
            return false;
        }

        file.close();
    } else {
        cout << filename << ": Unable to open" << endl;
        return false;
    }
    return true;
}

void quit(){
	delete cm;
	delete connection;
	delete crypto;

	cout << greetings << endl;
    exit(0);
}

/********************************/
/*      PROTOCOL FUNCTIONS      */
/********************************/

void cmd_help(){
    cout << endl;

    cout << "sftp: secure file transfer for file up to 4 GB" << endl;
    cout << "Usage:" << endl;
    cout << " help           -- show this content" << endl;
    cout << " rls            -- list all files on the remote server" << endl;
    cout << " lls [<path>]   -- list local files at the specified path" << endl;
    cout << " q              -- quit" << endl;
    cout << " get <filename> -- download the specified file" << endl;
    cout << " put <filename> -- upload the specified file" << endl;
    cout << " rm <filename>  -- remove the specified file" << endl;

    cout << endl;
}

void cmd_local_list(string path){
    /* retrieve list of files */
    ostringstream fileList;
    string type;
    string pwd;
    DIR* dd;
    dirent* de;

    if ( path.empty() ) path = ".";

    dd = opendir(path.c_str());
    if (dd) {
        while ((de = readdir(dd)) != NULL) {
            pwd = path + "/" + string(de->d_name);
            if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) { // if name is not . nor ..
                type = (is_file(pwd.c_str())) ? "-" : "d";
                fileList << type << " " << de->d_name << endl;
            }
        }
        closedir(dd);
        cout << fileList.str();
    } else {
        cout << path << ": No such directory" << endl;
    }
}

void cmd_remote_list(){
    string cmd = "LIST\n\n";
    send_cmd(cmd);

    recv_response();
    recv_list();
}

void cmd_quit(){
    string cmd = "QUIT\n\n";
    send_cmd(cmd);
	quit();
}


void cmd_retr(string filename){
    if ( filename.empty() ){
        cout << "usage: get <filename>" << endl;
        return;
    }
    string cmd = "RETR " + filename + "\n\n";
    send_cmd(cmd);

    recv_response();
    recv_file(filename);
}

void cmd_stor(string filepath){
    if ( filepath.empty() ){
        cout << "usage: put <filename>" << endl;
        return;
    }

    stringstream ss;
    string filename;
    int64_t filesize;

    /*
        getting the filename from the filepath (es. /etc/hosts)
        assuming that the user might store a file which is not
        in the current dir
    */
    ss << filepath;
    while(getline(ss, filename, '/'));

    // getting the file size
    if ( !check_and_get_file_size(filepath, filesize) ){
        return;
    }

    string cmd = "STOR " + filename + "\nSize: " + to_string(filesize) + "\n\n";

    send_cmd(cmd);
    recv_response();

    int response;
    is >> response;
    response = OK;
    if (response == OK){
        send_file(filepath, filename, filesize);
    } else {
        cerr << error << endl;
    }
}

void cmd_dele(string filename){
    if ( filename.empty() ){
        cout << "usage: rm <filename>" << endl;
        return;
    }
    string cmd = "DELE " + filename + "\n\n";

    send_cmd(cmd);
    recv_response();

    int response;
    is >> response;
    if (response == OK){
        cout << "File '" << filename << "' deleted successfully" << endl;
    } else if (response == BAD_FILE) {
        cout << filename << ": No such file on the server" << endl;
    } else {
        cerr << error << endl;
    }
}

void cmd_unknown(string cmd){
    cout << "error: '" << cmd << "' is an invalid command" << endl;
}

/****************************************/
/*                MAIN                  */
/****************************************/

int main(int argc, char* argv[]) {
    if (argc < 4){
        cout << "./bin/client <ipserver> <serverport> <certname>" << endl;
        exit(0);
    }

    string line;
    string sv_addr = argv[1];
    uint16_t sv_port = atoi(argv[2]);
	string cert_name = argv[3];

    try {
		cm = new CertManager(cert_name);
        // if (!cm) { debug(FATAL, "[F] cannot create Certificate Manager" << endl); exit(1); }

        connection = new Connection(sv_addr.c_str(), sv_port);
        // if (!connection) { debug(FATAL, "[F] cannot create Connection" << endl); exit(1); }

        // handshake
        if (!handshake()){ cout << "handshake error: Unable to connect to the server" << endl; exit(-1); }

        crypto = new Crypto(sessionKey, authKey, iv);

        while(1){
            // clear line, stringstream and input stream
            line.clear();
            is.clear();
            is.str("");
            cin.clear();

            cout << cursor;
            // waiting for command
            if (!getline(cin, line))
				line = "q";

            is.str(line);

            if (!line.empty()){
                parse_cmd();
            }
        }
    } catch(ExRecv e) {
        cout << "error: Unable to connect to server" << endl;
        cout << "info: You have been disconnected" << endl;
    } catch (Ex e) {
        cerr << e << endl;
    }

    quit();
}

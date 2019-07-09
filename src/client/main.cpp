#include "main.h"

/****************************************/
/*              SEND/RECV               */
/****************************************/
void send_cmd(string cmd){
    crypto->send(connection, cmd.c_str(), cmd.size());
}

void send_fragment(const char *buffer, const int len){
    crypto->send(connection, buffer, len);
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
        clog << "File '" << filename << "' stored successfully" << endl;
        file.close();
    } else {
        cerr << error << endl;
    }
}

void recv_response(){
    char buffer[BUFFER_SIZE];
    int recvBytes;
    char shiftRegister[2];

    memset(buffer, 0, BUFFER_SIZE);
    memset(shiftRegister, 0, 2);

    is.clear();
    is.str("");

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = crypto->recv(connection, buffer + i, 1);

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

    recvBytes = crypto->recv(connection, buffer, len);

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
            cerr << "[E] protocol error" << endl;
            return;
        }

        is >> msg_len;

        if (msg_len < 0){
            cerr << "[E] protocol error" << endl;
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
        cerr << "[E] protocol error" << endl;
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
            cerr << "[E] protocol error" << endl;
            return;
        }

        is >> filesize;

        if (filesize < 0){
            cerr << "[E] protocol error" << endl;
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
            clog << "File '" << filename << "' saved successfully in " << CLIENT_ROOT << endl;
        } else {
            cerr << "error: dir '" << CLIENT_ROOT << "' does not exist" << endl;
        }
    } else if (response == BAD_FILE) {
        cerr << filename << ": No such file" << endl;
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
    clog << "\b\b\b\b\b\b\b";

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
    clog << os.str() << flush;
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

bool check_and_get_file_size(string filename, int64_t& size){
    if (!is_file(filename)){
        cerr << filename << ": No such file" << endl;
        return false;
    }

    string err_msg;
    ifstream file;
    file.open(filename, ios::in | ios::binary | ios::ate);

    if (file.is_open()){
        // get the size because the cursor is at the end by means of ios::ate
        size = file.tellg();

        if (size > MAX_FILE_SIZE){
            err_msg = filename + ": exceeded maximum dimension (4 GiB)";
        }

        file.close();
    } else {
        err_msg = filename + ": Unable to open";
    }

    if (!err_msg.empty()){
        cerr << err_msg << endl;
        return false;
    }

    return true;
}

void quit(){
	delete cm;
	delete connection;
	delete crypto;

	clog << greetings << endl;
    exit(0);
}

/********************************/
/*      PROTOCOL FUNCTIONS      */
/********************************/

void cmd_help(){
    clog << endl;

    clog << "sftp: secure file transfer for file up to 4 GB" << endl;
    clog << "Usage:" << endl;
    clog << " help           -- show this content" << endl;
    clog << " rls            -- list all files on the remote server" << endl;
    clog << " lls [<path>]   -- list local files at the specified path" << endl;
    clog << " q              -- quit" << endl;
    clog << " get <filename> -- download the specified file" << endl;
    clog << " put <filename> -- upload the specified file" << endl;
    clog << " rm <filename>  -- remove the specified file" << endl;

    clog << endl;
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
        cerr << path << ": No such directory" << endl;
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
    recv_response();
	quit();
}


void cmd_retr(string filename){
    if ( filename.empty() ){
        cerr << "usage: get <filename>" << endl;
        return;
    }
    string cmd = "RETR " + filename + "\n\n";
    send_cmd(cmd);

    recv_response();
    recv_file(filename);
}

void cmd_stor(string filepath){
    if ( filepath.empty() ){
        cerr << "usage: put <filename>" << endl;
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
    if (response == OK){
        send_file(filepath, filename, filesize);
    } else {
        cerr << error << endl;
    }
}

void cmd_dele(string filename){
    if ( filename.empty() ){
        cerr << "usage: rm <filename>" << endl;
        return;
    }
    string cmd = "DELE " + filename + "\n\n";

    send_cmd(cmd);
    recv_response();

    int response;
    is >> response;
    if (response == OK){
        clog << "File '" << filename << "' deleted successfully" << endl;
    } else if (response == BAD_FILE) {
        cerr << filename << ": No such file on the server" << endl;
    } else {
        cerr << error << endl;
    }
}

void cmd_unknown(string cmd){
    cerr << "error: '" << cmd << "' is an invalid command" << endl;
}

vector<Key> handshake(void) {
    string buffer;
    debug(INFO, "[I] handshake with server..." << endl)

    /* === M1 === */
    /* send certificate */
    buffer = cm->getCert().str();
    debug(DEBUG, "[D] Client Certificate" << endl); vstrdump(DEBUG, buffer);
    connection->send(buffer);
    /* generate and send nonce */
    Nonce nonceClient;
    buffer = nonceClient.str();
    debug(DEBUG, "[D] Client Nonce" << endl); vstrdump(DEBUG, buffer);
    connection->send(buffer);

    /* === M2 === */
    /* receive and verify server's certificate */
    connection->recv(buffer);
    Certificate server_certificate; server_certificate.fromString(buffer);
    cm->verifyCert(server_certificate);

    /* receive encrypted keys, IVs and their signature, and server's nonce */
    connection->recv(buffer);
    RSASeal encrypted_session_key_seal; encrypted_session_key_seal.fromString(buffer);
    connection->recv(buffer);
    RSASeal encrypted_auth_key_seal; encrypted_auth_key_seal.fromString(buffer);
    connection->recv(buffer);
    Key iv(buffer);
    connection->recv(buffer);
    string signature = buffer;
    connection->recv(buffer);
    Nonce nonceServer(buffer);

    /* create asymmetric key cryptosystem */
    RSACrypto rsacrypto(cm->getCert(), cm->getPrivKey());

    /* verify server's signature */
    string what_to_verify = encrypted_session_key_seal.str() + encrypted_auth_key_seal.str() + iv.str() + nonceClient.str();
    RSAKey server_public_key; server_public_key.fromCertificate(server_certificate);
    rsacrypto.verify(what_to_verify, signature, server_public_key);
    debug(DEBUG, "[D] server's signature is valid" << endl);

    /* decrypt symmetric session+auth keys */
    Key session_key(rsacrypto.decrypt(encrypted_session_key_seal));
    Key auth_key(rsacrypto.decrypt(encrypted_auth_key_seal));

    vector<Key> keys;
    keys.push_back(session_key);
    keys.push_back(auth_key);
    keys.push_back(iv);

    /* === M3 === */
    /* sign server's nonce and send signature back */
    string what_to_sign = nonceServer.str();
    signature = rsacrypto.sign(what_to_sign);
    connection->send(signature);

    return keys;
}

/****************************************/
/*                MAIN                  */
/****************************************/

int main(int argc, char* argv[]) {
    if (argc < 4){
        clog << "./bin/client <ipserver> <serverport> <username> [DEBUG]" << endl;
        exit(0);
    }

    string line;
    string sv_addr = argv[1];
    uint16_t sv_port = atoi(argv[2]);
	string username = argv[3];
    vector<string> authServersList;
    authServersList.push_back("server");

    /* enable debug output */
    if (argc == 5) debugenable(argv[4]);

    try {
		cm = new CertManager(username, authServersList);

        connection = new Connection(sv_addr.c_str(), sv_port);

        // handshake with server
        vector<Key> keys = handshake();

        crypto = new Crypto(keys.at(0), keys.at(1), keys.at(2));

        while(1){
            // clear line, stringstream and input stream
            line.clear();
            is.clear();
            is.str("");
            cin.clear();

            clog << cursor << flush;
            // waiting for command
            getline(cin, line);
            if (!cin) line = "q";

            is.str(line);

            if (!line.empty()){
                parse_cmd();
            }
        }
    } catch(ExNetwork e) {
        cerr << "[E] network: " << e << endl;
    } catch (Ex e) {
        cerr << "[E] exception: " << e << endl;
    }

    cerr << error << endl;
    quit();
}

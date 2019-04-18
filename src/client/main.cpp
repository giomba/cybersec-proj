#include "client.h"

using namespace std;

/****************************************/
/*              SEND/RECV               */
/****************************************/

void send_cmd(){
    string str_cmd, filename;
    is >> str_cmd >> filename;

    CommandType cmd = str2cmd(str_cmd);

    switch(cmd){
        case HELP:   cmd_help(); break;
        case QUIT:   cmd_quit(); break;
        case L_LIST: cmd_local_list(); break;
        case R_LIST: cmd_remote_list(); break;
        case RETR:   cmd_retr(filename); break;
        case STOR:   cmd_stor(filename); break;
        case DELE:   cmd_dele(filename); break;
        default:     cmd_unknown(str_cmd);
    }
}

void recv_response(){
    char buffer[BUFFER_SIZE];
    int recvBytes;
    char shiftRegister[2];

    memset(buffer, 0, BUFFER_SIZE);
    is.clear();

    for (int i = 0; i < BUFFER_SIZE - 1; ++i) {
        recvBytes = connection->recv(buffer + i, 1);
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

void recv_list(){
    char buffer[BUFFER_SIZE];
    int response;
    string attr;
    size_t msg_len;

    is >> response;
    if (response == OK){
        is >> attr >> msg_len;

        memset(buffer, 0, BUFFER_SIZE);
        connection->recv(buffer, msg_len);

        is.clear();
        is.str(string(buffer));
        string file;
        while(is >> file){
            cout << "   - 📄 " << file << endl;
        }
    }
}

void recv_file(){
    cout << "Receiving file..." << endl;
}

/********************************/
/*            UTILS             */
/********************************/

CommandType str2cmd(string str){
    if (str.compare("help") == 0)   { return HELP; }
    if (str.compare("lls") == 0)    { return L_LIST; }
    if (str.compare("rls") == 0)    { return R_LIST; }
    if (str.compare("q") == 0)      { return QUIT; }
    if (str.compare("get") == 0)    { return RETR; }
    if (str.compare("put") == 0)    { return STOR; }
    if (str.compare("rm") == 0)     { return DELE; }
    return BAD_REQ;
}

/********************************/
/*      PROTOCOL FUNCTIONS      */
/********************************/

void cmd_help(){
    cout << endl;

    cout << "sftp: secure file transfer for file up to 4 GB" << endl;
    cout << "Usage:" << endl;
    cout << " help           -- show this content" << endl;
    cout << " rls            -- list all files available on the server" << endl;
    cout << " lls [<path>]   -- list local files at the specified path" << endl;
    cout << " q              -- quit" << endl;
    cout << " get <filename> -- download the specified file" << endl;
    cout << " put <filename> -- upload the specified file" << endl;
    cout << " rm <filename>  -- remove the specified file" << endl;

    cout << endl;
}

void cmd_local_list(){
    if ( ::execve("/bin/ls", NULL, NULL) != 0 ){
        perror("error:");
    }
}

void cmd_remote_list(){
    string cmd = "LIST\n\n";
    connection->send(cmd.c_str(), cmd.length());
    //debug(DEBUG, cmd);
    recv_response();
    recv_list();
}

void cmd_quit(){
    cout << greetings << endl;
    exit(0);
}

void cmd_allo(string filename){
    if (filename == ""){
        cout << "usage: allo <filename>" << endl;
        return;
    }
    string cmd = "ALLO " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    connection->send(cmd.c_str(), cmd.length());
}

void cmd_retr(string filename){
    if (filename == ""){
        cout << "usage: get <filename>" << endl;
        return;
    }
    string cmd = "RETR " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    connection->send(cmd.c_str(), cmd.length());

    recv_response();
    recv_file();
}

void cmd_stor(string filepath){
    if (filepath == ""){
        cout << "usage: put <filename>" << endl;
        return;
    }

    stringstream ss;
    string filename;
    string filesize;

    /*
        getting the filename from the filepath (es. /etc/hosts)
        assuming that the user might store a file which is not
        in the current dir
    */
    ss << filepath;
    while(getline(ss, filename, '/'));

    // clear stringstream
    ss.clear();
    ss.str("");

    // getting the file size
    ss << read(filepath);
    filesize = ss.str();
    string cmd = "STOR " + filename + "\nSize: " + filesize + "\n\n";

    debug(INFO, cmd.c_str());
    //connection->send(cmd.c_str(), cmd.length());
    recv_response();
}

void cmd_dele(string filename){
    if (filename == ""){
        cout << "usage: rm <filename>" << endl;
        return;
    }
    string cmd = "DELE " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    //connection->send(cmd.c_str(), cmd.length());

    recv_response();
}

void cmd_unknown(string cmd){
    cout << "error: '" << cmd << "' is an invalid command" << endl;
}

size_t read(string filename){
    streampos size;
    char *memblock = 0;

    ifstream file;
    file.open(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()){
        size = file.tellg();
        cout << "Copying " << size << " bytes in memory\n";
        memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();
        cout << memblock << endl;
    } else {
        cout << "Unable to open file" << endl;
    }
    return size;
}

/****************************************/
/*                MAIN                  */
/****************************************/

int main(int argc, char* argv[], char *envp[]) {
    if (argc < 3){
        cout << "./bin/client <ipserver> <serverport>" << endl;
        exit(0);
    }

    string line;

    string sv_addr = argv[1];
    uint16_t sv_port = atoi(argv[2]);

    try {
        connection = new Connection(sv_addr.c_str(), sv_port);

        while(1){
            // clear line, stringstream and input stream
            line.clear();
            is.clear();
            is.str("");
            cin.clear();

            cout << cursor;
            // waiting for command
            getline(cin, line);
            is.str(line);

            if (!line.empty()){
                send_cmd();
            }
        }
    } catch(Ex e) {
        cerr << e;
    }
}

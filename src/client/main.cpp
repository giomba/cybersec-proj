#include "client.h"

using namespace std;

/****************************************/
/*              SEND/RECV               */
/****************************************/

void send_cmd(){
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
            cout << file << endl;
        }
    }
}

void recv_file(){
    int response;
    is >> response;
    if (response == OK){
        cout << "Receiving file..." << endl;
    } else {
        cout << error << endl;
    }
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
    DIR* dd;
    dirent* de;

    if ( path.empty() ) path = ".";

    dd = opendir(path.c_str());
    if (dd) {
        while ((de = readdir(dd)) != NULL) {
            if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") /*&& strstr(de->d_name, ".") != de->d_name*/) { // if name is not . nor ..
                fileList << de->d_name << endl;
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
    if ( filename.empty() ){
        cout << "usage: allo <filename>" << endl;
        return;
    }
    string cmd = "ALLO " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    connection->send(cmd.c_str(), cmd.length());
}

void cmd_retr(string filename){
    if ( filename.empty() ){
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
    if ( filepath.empty() ){
        cout << "usage: put <filename>" << endl;
        return;
    }

    stringstream ss;
    string filename;
    size_t filesize;

    /*
        getting the filename from the filepath (es. /etc/hosts)
        assuming that the user might store a file which is not
        in the current dir
    */
    ss << filepath;
    while(getline(ss, filename, '/'));

    // getting the file size
    bool ret;
    if ( !(ret = read(filepath, filesize)) ){
        return;
    }

    string cmd = "STOR " + filename + "\nSize: " + to_string(filesize) + "\n\n";

    debug(INFO, cmd.c_str());
    //connection->send(cmd.c_str(), cmd.length());
    //recv_response();

    int response;
    is >> response;
    response = OK;
    if (response == OK){
        cout << "File '" << filename << "' stored successfully" << endl;
    } else {
        cout << error << endl;
    }
}

void cmd_dele(string filename){
    if ( filename.empty() ){
        cout << "usage: rm <filename>" << endl;
        return;
    }
    string cmd = "DELE " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    //connection->send(cmd.c_str(), cmd.length());

    //recv_response();

    int response;
    is >> response;
    response = OK;
    if (response == OK){
        cout << "File '" << filename << "' deleted successfully" << endl;
    } else {
        cout << error << endl;
    }
}

void cmd_unknown(string cmd){
    cout << "error: '" << cmd << "' is an invalid command" << endl;
}

bool read(string filename, size_t &size){
    char *memblock = 0;

    ifstream file;
    file.open(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()){
        // get the size because the cursor is at the end by means of ios::ate
        size = file.tellg();

        if (size > MAX_FILE_SIZE){
            cout << filename << ": exceeded maximum dimension (4 GB)" << endl;
            return false;
        }

        cout << "Copying " << size << " bytes in memory\n";
        memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();
        cout << memblock << endl;
        free(memblock);
    } else {
        cout << filename << ": No such file" << endl;
        return false;
    }
    return true;
}

/****************************************/
/*                MAIN                  */
/****************************************/

int main(int argc, char* argv[]) {
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

#include "client.h"

using namespace std;

/****************************************/
/*              SEND/RECV               */
/****************************************/

void send_cmd(string cmd){
    connection->send(cmd.c_str(), cmd.length());
}

void send_file(string filepath, string filename, size_t size){
    char buffer[BUFFER_SIZE];
    ifstream file;
    uint64_t remainingBytes = size;
    
    file.open(filepath, ios::in | ios::binary);
    if (file.is_open()){
        while( remainingBytes > 0 ){
            if ( remainingBytes > BUFFER_SIZE ){
                file.read(buffer, BUFFER_SIZE);
                remainingBytes -= BUFFER_SIZE;
            } else {
                file.read(buffer, remainingBytes);
                remainingBytes = 0;
            }
            connection->send(buffer, BUFFER_SIZE);
        }
        cout << "File '" << filename << "' stored successfully" << endl;
        file.close();
    } else {
        cout << error << endl;
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

void recv_file(string filename){
    char buffer[BUFFER_SIZE];
    int response;
    string attr;
    size_t filesize;
    size_t recvBytes = 0;
    
    is >> response;
    if (response == OK){
        is >> attr >> filesize;

        while(recvBytes < filesize){
            recvBytes += connection->recv(buffer + recvBytes, filesize);
        }
        
        ofstream file;
        file.open(CLIENT_ROOT + filename, ios::out | ios::binary);
        if (file.is_open()){
            file.write(buffer, recvBytes);
            file.close();
            cout << "File '" << filename << "' saved successfully in " << CLIENT_ROOT << endl;
        } else {
            cout << "error: dir '" << CLIENT_ROOT << "' does not exist" << endl;
        }
    } else if (response == BAD_FILE) {
        cout << filename << ": No such file" << endl;
    } else
        cout << error << endl;
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
    send_cmd(cmd);

    recv_response();
    recv_list();
}

void cmd_quit(){
    cout << greetings << endl;
    exit(0);
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
    size_t filesize;

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
        cout << error << endl;
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
    } else {
        cout << error << endl;
    }
}

void cmd_unknown(string cmd){
    cout << "error: '" << cmd << "' is an invalid command" << endl;
}

bool check_and_get_file_size(string filename, size_t &size){
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
                parse_cmd();
            }
        }
    } catch(Ex e) {
        cerr << e;
    }
}

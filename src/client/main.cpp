/*
#include "const.h"
#include "cmd.h"
*/
#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#define DEFAULT_SIZE 64

using namespace std;

char buffer[DEFAULT_SIZE];
Connection *c;

enum CommandType {
    HELP, LIST, QUIT, RETR, STOR, DELE, BAD_REQ
};

CommandType str2cmd(string str){
    if (str.compare("help") == 0)   { return HELP; }
    if (str.compare("ls") == 0)     { return LIST; }
    if (str.compare("q") == 0)      { return QUIT; }
    if (str.compare("get") == 0)    { return RETR; }
    if (str.compare("put") == 0)    { return STOR; }
    if (str.compare("rm") == 0)     { return DELE; }
    return BAD_REQ;
}

void availableCommands(){
    cout << endl;

    cout << "sftp: secure file transfer for file up to 4 GB" << endl;
    cout << "Usage:" << endl;
    cout << " help           -- show this content" << endl;
    cout << " ls             -- list all files available on the server" << endl;
    cout << " q              -- quit" << endl;
    cout << " get <filename> -- download the specified file" << endl;
    cout << " put <filename> -- upload the specified file" << endl;
    cout << " rm <filename>  -- remove the specified file" << endl;

    cout << endl;
}

vector<string> split(string str, char delimeter){
    stringstream ss(str);
    string s;
    vector<string> str_vector;
    while (getline(ss, s, delimeter)){
        str_vector.push_back(s);
    }
    return str_vector;
}

/********************************/
/*      PROTOCOL FUNCTIONS      */
/********************************/

void list(){
    string cmd = "LIST\n\n";
    debug(INFO, "sending LIST request...");
    c->send(cmd.c_str(), cmd.length());
}

void quit(){
    string greetings = "Bye :(";
    cout << greetings << endl;
    exit(0);
}

void retrieve(string filename){
    if (filename == ""){
        debug(ERROR, "RETR <filename>");
        return;
    }
    string cmd = "RETR " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    c->send(cmd.c_str(), filename.length());
}

void store(string filename){
    if (filename == ""){
        debug(ERROR, "STOR <filename>");
        return;
    }
    //string cmd = "STOR " + filename + "\nSize: " + filesize + "\n\n";
    string cmd = "STOR " + filename;
    debug(INFO, cmd.c_str());
    c->send(cmd.c_str(), filename.length());
}

void deleteFile(string filename){
    if (filename == ""){
        debug(ERROR, "DELE <filename>");
        return;
    }
    string cmd = "DELE " + filename + "\n\n";
    debug(INFO, cmd.c_str());
    c->send(cmd.c_str(), filename.length());
}

void parseCommand(string str){
    vector<string> parsed_str = split(str, ' ');
    /*
    for (int i = 0; i < parsed_str.size(); i++){
        cout << parsed_str[i] << endl;
    }
    */
    CommandType cmd = str2cmd(parsed_str[0]);

    switch(cmd){
        case HELP: availableCommands(); break;
        case LIST: cout << "LIST" << endl; list(); break;
        case QUIT: cout << "QUIT" << endl; quit(); break;
        case RETR: cout << "RETR" << endl; retrieve(parsed_str[1]); break;
        case STOR: cout << "STOR" << endl; store(parsed_str[1]); break;
        case DELE: cout << "DELE" << endl; deleteFile(parsed_str[1]); break;
        default:   cout << "BAD_REQ" << endl;
    }
}

void read(const char *filename){
    streampos size;
    char *memblock = 0;

    ifstream file;
    file.open(filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()){
        size = file.tellg();
        cout << "Copying " << size << " bytes in memory\n";
        memblock = new char [size];
        file.seekg (0, ios::beg);
        file.read (memblock, size);
        file.close();
        cout << memblock;
    } else {
        cout << "Unable to open file" << endl;
    }
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
    string banner = "Ciao Hello Salut Hallo Nihao";

    char* sv_addr = argv[1];
    int sv_port = atoi(argv[2]);

    try {
        c = new Connection(sv_addr, sv_port);
        cout << banner << endl;
        while(1){
            // pulizia buffer
            line.clear();
            cout << ">";
            // waiting for command
            cin.clear();
            getline(cin, line);

            if (!line.empty()){
                parseCommand(line);
            }
        }
    } catch(Ex e) {
        cerr << e;
    }
}

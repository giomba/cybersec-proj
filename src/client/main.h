#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>

#include "../common/certmanager.h"
#include "../common/connection.h"
#include "../common/crypto.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/key.h"
#include "../common/nonce.h"
#include "../common/protocol.h"
#include "../common/rsacrypto.h"
#include "../common/rsaseal.h"

#define CLIENT_ROOT "downloads/"

#define cursor      "> "
#define greetings   "Bye :("
#define error       "Something went wrong"

using namespace std;

istringstream is;
Connection* connection;
Crypto* crypto;
CertManager* cm;

enum CommandType {
    HELP, R_LIST, L_LIST, QUIT, RETR, STOR, DELE, BAD_REQ
};

void send_cmd(string);
void send_fragment(const char*, const int);
void send_file(string, string, int64_t);
int recv_fragment(char*, const int);
void recv_response();
void recv_list();
void recv_file(string);

CommandType str2cmd(string);
void show_progress(double);
void parse_cmd(string);
bool is_file(string);
bool check_and_get_file_size(string, int64_t&);

void cmd_help();
void cmd_local_list(string);
void cmd_quit();
void cmd_remote_list();
void cmd_allo(string);
void cmd_stor(string);
void cmd_retr(string);
void cmd_dele(string);
void cmd_unknown(string);

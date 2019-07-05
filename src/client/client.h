#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>

#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/protocol.h"
#include "../common/crypto.h"
#include "../common/certmanager.h"
#include "../common/signer.h"


#define CLIENT_ROOT "downloads/"

#define cursor      "> "
#define greetings   "Bye :("
#define error       "Something went wrong. Please retry later."

istringstream is;
Connection* connection;
Crypto* crypto;
CertManager* cm;
Signer* signer;

/* TODO 
unsigned char* sessionKey;
unsigned char* authKey;
unsigned char* iv;
*/

unsigned char sessionKey[] = "1230000000000321";
unsigned char authKey[] = "0123456789abcdef9876543210abcdef";
unsigned char iv[] = "0000000000000000";

enum CommandType {
    HELP, R_LIST, L_LIST, QUIT, RETR, STOR, DELE, BAD_REQ
};

int handshake();
int sendM1();
int receiveM2();
int sendM3();

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

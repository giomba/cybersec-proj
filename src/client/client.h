#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#include "../common/connection.h"
#include "../common/debug.h"
#include "../common/exception.h"
#include "../common/protocol.h"

#define BUFFER_SIZE 4 * (1 << 10)

const string cursor = "👉 ";
const string greetings = "👋 Bye";

Connection *connection;
istringstream is;

enum CommandType {
    HELP, R_LIST, L_LIST, QUIT, RETR, STOR, DELE, BAD_REQ
};

void send_cmd();
void recv_response();
void recv_list();
void recv_file();

CommandType str2cmd(string);
size_t read(string);

void cmd_help();
void cmd_local_list();
void cmd_quit();
void cmd_remote_list();
void cmd_allo(string);
void cmd_stor(string);
void cmd_retr(string);
void cmd_dele(string);
void cmd_unknown(string);

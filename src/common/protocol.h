#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <regex>

using namespace std;

#define OK                          200
#define BAD_FILE                    452
#define SERVER_ERROR                500
#define SYNTAX_ERROR                501
#define COMMAND_NOT_IMPLEMENTED     502
#define BAD_SEQUENCE_OF_COMMANDS    503

extern regex parola;

#endif

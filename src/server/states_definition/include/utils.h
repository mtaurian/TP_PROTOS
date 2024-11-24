#ifndef TP_PROTOS_UTILS_H
#define TP_PROTOS_UTILS_H

#include <ctype.h>
#include "../../include/pop3.h"

#define STATES_AMOUNT 4
#define COMMAND_AMOUNT 9
#define AUTH_COMMAND_AMOUNT 2
#define TRANSACTION_COMMAND_AMOUNT 7
#define UPDATE_COMMAND_AMOUNT 1
#define CAPS 32
#define MAX_COMMAND_SIZE 4

typedef enum boolean {
    FALSE = 0,
    TRUE = 1
} boolean;

typedef enum commands {
    USER = 0, PASS, STAT, LIST, RETR, DELE, NOOP, RSET, QUIT
} commands;

typedef struct command_struct {
    char * string;
    commands command;
} command_struct; 

typedef struct possible_command_struct {
    char * string;
    commands command;
    boolean possible;
    boolean has_params;
} possible_command_struct;

typedef struct user_request {
    commands command;
    boolean is_valid;
    char * arg;
} user_request;

/*
    Parses user's request and returns a user_request or NULL if command not found
*/
user_request * parse(struct selector_key * key, pop3_states state);

void write_std_response(char isOk, char * msg, struct selector_key * key);


/*
    Converts a string to lower case
*/
char * toLower(char * str);


#endif //TP_PROTOS_UTILS_H

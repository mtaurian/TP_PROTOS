#ifndef TP_PROTOS_UTILS_H
#define TP_PROTOS_UTILS_H

#include <ctype.h>
#include "../../include/pop3.h"
#include <stdio.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 50
#define MAX_ARGUMENTS 10
#define COMMAND_AMOUNT 10


typedef enum boolean {
    FALSE = 0,
    TRUE = 1
} boolean;

typedef enum commands {
    USER = 0, PASS, STAT, LIST, RETR, DELE, NOOP, RSET, QUIT, INVALID
} commands;

typedef struct command_struct {
    char * string;
    commands command;
    boolean has_params;
} command_struct;

typedef struct user_request {
    commands command;
    boolean is_valid;
    char arg[BUFFER_SIZE];
} user_request;

/*
    Parses user's request and returns a user_request or NULL if command not found
*/
user_request * parse(struct selector_key * key);

void write_std_response(char isOk, char * msg, struct selector_key * key);


/*
    Converts a string to lower case
*/
char * toLower(char * str);


#endif //TP_PROTOS_UTILS_H

#ifndef TP_PROTOS_UTILS_H
#define TP_PROTOS_UTILS_H

#include <ctype.h>
#include "../../server/pop3/include/pop3.h"
#include <stdio.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 50
#define MAX_ARGUMENTS 10
#define COMMAND_AMOUNT 10
#define MAX_MESSAGE_LEN 128
#define OK 1
#define ERR 0


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

typedef struct client_data {
    struct sockaddr_storage clientAddress;
    bool closed;
    int clientFd;

    buffer clientBuffer;
    uint8_t inClientBuffer[BUFFER_SIZE];

    buffer responseBuffer;
    uint8_t inResponseBuffer[BUFFER_SIZE];

    char * username;
    char * password;

    user_data * user;
    struct state_machine stm;

} client_data;

typedef enum errors {
    NO_USERNAME_GIVEN = 0,
    AUTHENTICATION_FAILED,
    INVALID_MESSAGE_NUMBER,
    NO_MESSAGE,
    UNKNOWN_COMMAND,
    NOICE_AFTER_MESSAGE,
    MESSAGE_ALREADY_DELETED
} errors;

typedef struct complete_error {
	enum errors type;
	char * message;
} complete_error;

/*
    Parses user's request and returns a user_request or NULL if command not found
*/
user_request * parse(struct selector_key * key);

void write_std_response(char isOk, char * msg, struct selector_key * key);

void read_handler(struct selector_key *_key);
void write_handler(struct selector_key *_key);


/*
    Converts a string to lower case
*/
char * toLower(char * str);

/*
    Function that unifies the error messages
*/
void write_error_message(struct selector_key * key, enum errors error);

/*
    Function that unifies the error messages with an extra argument
*/
void write_error_message_with_arg(struct selector_key * key, enum errors error, char * extra);


#endif //TP_PROTOS_UTILS_H

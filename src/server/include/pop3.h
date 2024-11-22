#ifndef POP3_H
#define POP3_H


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "../../shared/include/buffer.h"
#include "../../shared/include/stm.h"

#define ATTACHMENT(key) ((client_data*)(key)->data)
#define BUFFER_SIZE 2048

#define MAX_USERS 10

struct users {
    char *name;
    char *pass;
    unsigned int logged;
};


typedef struct client_data {
    struct sockaddr_storage clientAddress;
    bool closed;
    int clientFd;

    struct buffer clientBuffer;
    uint8_t inClientBuffer[BUFFER_SIZE];

    struct buffer responseBuffer;
    uint8_t inResponseBuffer[BUFFER_SIZE];

    char * username;
    char * password;
    struct state_machine stm;

} client_data;


struct pop3_server {
    struct users users_list[MAX_USERS];
    unsigned int user_amount;
};

enum pop3_states {
    AUTHORIZATION_USER = 0, AUTHORIZATION_PASSWORD, TRANSACTION, UPDATE
};

void initialize_pop3_server();
void free_pop3_server();

void pop3_passive_accept(struct selector_key *_key);

void close_client(struct selector_key *_key);
void read_handler(struct selector_key *_key);
void write_handler(struct selector_key *_key);

void user(char *s);
void log_out_user(struct users *user);
void log_user(struct users *user);
unsigned int validate_user(char *username, char *password);

#endif //POP3_H

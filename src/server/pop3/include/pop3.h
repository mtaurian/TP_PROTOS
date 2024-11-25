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

#include "../../../shared/include/buffer.h"
#include "../../../shared/include/stm.h"

#define BUFFER_SIZE 2048
#define MAX_MAILS 100
#define MAILDIR_PERMISSIONS 0777
#define DIR_AMOUNT 3

typedef struct mail {
    int id;
    int fd;
    char *filename;
    size_t size;
    int deleted; // TODO: change unsigned int
} mail;

typedef struct t_mailbox {
    mail *mails;
    size_t mails_size; // size sum of mails not marked as deleted
    int mail_count; // mails that are not marked as deleted
    int deleted_count;
} t_mailbox;

#define MAX_USERS 505

typedef struct user_data {
    char *name;
    char *pass;
    unsigned int logged;
    t_mailbox *mailbox;
} user_data;

struct pop3_server {
    user_data users_list[MAX_USERS];
    unsigned int user_amount;
    char* maildir;
    size_t historic_connections; //volatile :/
    pthread_mutex_t  hc_mutex; // historic connections mutex
};

typedef enum pop3_states {
    AUTHORIZATION_USER = 0, AUTHORIZATION_PASSWORD, TRANSACTION, UPDATE
} pop3_states;



void initialize_pop3_server();
void free_pop3_server();

void pop3_passive_accept(struct selector_key *_key);
void close_client(struct selector_key *_key);

void user(char *s);
void log_out_user(user_data *user);
unsigned int log_user(user_data *user);
user_data * validate_user(char *username, char *password);
void free_user_data(user_data *user);
void set_maildir(char *maildir);
unsigned int load_mailbox(user_data *user);
void free_mailbox(t_mailbox* mails);


// for management server
size_t get_historic_connections();
user_data * get_users();
size_t get_users_amount();


// could be in a utils file
size_t get_file_size(const char *filename);
#endif //POP3_H

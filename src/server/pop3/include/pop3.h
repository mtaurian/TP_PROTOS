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
#define MAX_USERS 505
#define INITIAL_ACCESS_SIZE 60

typedef struct mail {
    int id;
    int fd;
    char *filename;
    size_t size;
    unsigned int deleted;
} mail;

typedef struct t_mailbox {
    mail *mails;
    size_t mails_size; // size sum of mails not marked as deleted
    int mail_count; // mails that are not marked as deleted
    int deleted_count;
} t_mailbox;

typedef struct user_data {
    char *name;
    char *pass;
    unsigned int logged;
    t_mailbox *mailbox;
    unsigned char to_delete; // Flag to delete the user, when user is logged
} user_data;

typedef enum access_type {
    LOGIN_ACCESS = 0, LOGOUT_ACCESS
} access_type;

typedef struct access_log {
    user_data *user;
    time_t access_time;
    access_type type;
} access_log;

struct pop3_server {
    user_data * users_list[MAX_USERS];
    unsigned int user_amount;
    char* maildir;

    char* transformation;

    size_t historic_connections; //volatile :/
    size_t bytes_transferred;
    access_log **log;
    size_t log_size;
};

typedef enum pop3_states {
    INITIAL = 0, AUTHORIZATION_USER, AUTHORIZATION_PASSWORD, TRANSACTION, UPDATE
} pop3_states;



void initialize_pop3_server();
void free_pop3_server();

void pop3_passive_accept(struct selector_key *_key);
void close_client(struct selector_key *_key);

unsigned char user(char *s);
void log_out_user(user_data *user);
unsigned int log_user(user_data *user);
user_data *validate_user(char *username, char *password);
void free_user_data(user_data *user);
void set_maildir(char *maildir);
void set_transformation(const char *transformation);

unsigned int load_mailbox(user_data *user);
void free_mailbox(t_mailbox* mails);
char * get_maildir();

// for management server
unsigned int get_current_connections();
size_t get_bytes_transferred();
size_t get_historic_connections();
user_data ** get_users();
size_t get_users_amount();
unsigned char add_user(char * user_and_pass);
unsigned char validate_user_not_exists(char * username);
unsigned char delete_user(char * username);
void add_bytes_transferred(size_t bytes);
access_log ** get_access_log();
size_t get_log_size();

// could be in a utils file
size_t get_file_size(const char *filename);

int transform_mail_piping(int mail_fd);

#endif //POP3_H

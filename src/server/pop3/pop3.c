#include "include/pop3.h"
#include "states_definition/include/initial.h"
#include "states_definition/include/auth_user.h"
#include "states_definition/include/auth_pass.h"
#include "states_definition/include/transaction.h"
#include "states_definition/include/update.h"
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

static struct pop3_server * server;

static const struct fd_handler client_handler = {
    .handle_read   =  read_handler,
    .handle_write  =  write_handler,
    .handle_close  = close_client,
    .handle_block = NULL,
};

static const struct state_definition states[] = {
    {
        .state            = INITIAL,
        .on_arrival       = initial_on_arrival,
        .on_departure     = initial_on_departure,
        .on_read_ready    = NULL,
        .on_write_ready   = initial_on_ready_to_write,
    },
    {
        .state            = AUTHORIZATION_USER,
        .on_arrival       = auth_user_on_arrival,
        .on_departure     = auth_user_on_departure,
        .on_read_ready    = auth_user_on_ready_to_read,
        .on_write_ready   = auth_user_on_ready_to_write,
    },
    {
        .state            = AUTHORIZATION_PASSWORD,
        .on_arrival       = auth_pass_on_arrival,
        .on_departure     = auth_pass_on_departure,
        .on_read_ready    = auth_pass_on_ready_to_read,
        .on_write_ready   = auth_pass_on_ready_to_write,
    },
    {
        .state            = TRANSACTION,
        .on_arrival       = transaction_on_arrival,
        .on_departure     = transaction_on_departure,
        .on_read_ready    = transaction_on_ready_to_read,
        .on_write_ready   = transaction_on_ready_to_write,
    },
    {
        .state            = UPDATE,
        .on_arrival       = update_on_arrival,
        .on_departure     = NULL,
        .on_read_ready    = update_on_ready_to_read,
        .on_write_ready   = update_on_ready_to_write,
    }
};

void initialize_pop3_server() {
    server = malloc(sizeof(struct pop3_server));
    server->user_amount = 0;
    server->bytes_transferred = 0;
    server->maildir = NULL;
    server->transformation = NULL;

    server->historic_connections = 0;
    server->log = NULL;
    server->log = malloc(INITIAL_ACCESS_SIZE*sizeof(access_log*));
    server->log_size = 0;
}

void free_pop3_server() {
    for (int i = 0; i < server->user_amount; i++) {
        free_user_data(server->users_list[i]);
    }

    if(server->maildir != NULL) {
        free(server->maildir);
    }

    if(server->transformation != NULL) {
        free(server->transformation);
    }

    for (int i = 0; i < server->log_size; i++) {
        free(server->log[i]);
    }
    free(server->log);


    free(server);
}

void free_user_data(user_data *user) {
    if (user->name != NULL) {
        free(user->name );
    }
    if (user->pass!= NULL) {
        free(user->pass);
    }

    if (user->logged && user->mailbox != NULL) {
        free_mailbox(user->mailbox);
        user->mailbox = NULL;
    }
    free(user);
}

void pop3_passive_accept(struct selector_key *_key) {
    const char *err_msg = NULL;
    struct sockaddr_storage client_addr;
    selector_status ss = SELECTOR_SUCCESS;
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    int client_fd = accept(_key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
        err_msg = "Error in accept";
        goto finally;
    }

    if (selector_fd_set_nio(client_fd) == -1) {
        err_msg = "Unable to set client socket flags";
        goto finally;
    }

    client_data* clientData = calloc(1, sizeof(client_data));
    if (clientData == NULL) {
        return;
    }

    clientData->closed = false;
    clientData->readyToLogout = false;
    clientData->clientFd = client_fd;
    clientData->clientAddress = client_addr;
    clientData->username = NULL;
    clientData->password = NULL;
    clientData->user = NULL;

    clientData->stm.states = states;
    buffer_init(&clientData->clientBuffer, BUFFER_SIZE, clientData->inClientBuffer);
    buffer_init(&clientData->responseBuffer, BUFFER_SIZE, clientData->inResponseBuffer);

    clientData->stm.initial = INITIAL;
    clientData->stm.max_state = UPDATE;
    stm_init(&clientData->stm);
    ss = selector_register(_key->s, client_fd, &client_handler, OP_WRITE, clientData);
    if (ss != SELECTOR_SUCCESS) {
        err_msg = "[POP3] Unable to register client socket handler";
    }

    //manager metrics
    server->historic_connections++;

    finally:
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n",  err_msg,
                ss == SELECTOR_IO
                ? strerror(errno)
                : selector_error(ss));
    } else if(err_msg) {
        perror(err_msg);
        free(clientData);
        close(client_fd);
    } else {
        printf("[POP3] Client connected\n");
    }

}

void close_client(struct selector_key * _key) {
    client_data* data = ATTACHMENT(_key);
    if (data->closed)
        return;

    data->closed = true;

    int clientFd = data->clientFd;

    if (clientFd != -1) {
        selector_unregister_fd(_key->s, clientFd);
        close(clientFd);
    }

    if (data->user != NULL) {
        log_out_user(data->user);
    }

    free(data->password);
    free(data->username);
    free(data);

    printf("[POP3] Client closed connection.\n");
}

unsigned char validate_user_not_exists(char * username){
    for(int i = 0; i < server->user_amount; i++) {
        if(strcmp(server->users_list[i]->name, username) == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

unsigned char user(char *s) {
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        return 0;
    } else {
        *p = 0;
        p++;
        size_t name_length = strlen(s);
        size_t pass_length = strlen(p);
        char * name = malloc(sizeof(char)*(name_length+1));
        char * pass = malloc(sizeof(char)*(pass_length+1));
        strcpy(name, s);
        strcpy(pass, p);
        if (validate_user_not_exists(name)) {
            server->users_list[server->user_amount] = malloc(sizeof(user_data));
            user_data * user = server->users_list[server->user_amount];
            user->pass = pass;
            user->name = name;
            user->logged = 0;
            user->to_delete = 0;
            server->user_amount++;
            return TRUE;
        }
        free(name);
        free(pass);
        return FALSE;
    }
}

void set_maildir(char *maildir) {
    server->maildir = malloc(PATH_MAX);
    strcpy(server->maildir, maildir);
    printf("[POP3] MAILDIR: %s\n", server->maildir);
}

void set_transformation(const char *transformation) {
    server->transformation = malloc(PATH_MAX);
    strcpy(server->transformation, transformation);
}

void new_access_log(user_data* user, access_type access_type) {
    server->log_size++;

    if(sizeof(server->log) == server->log_size) {
        access_log **new_log = realloc(server->log, (server->log_size + INITIAL_ACCESS_SIZE) * sizeof(access_log*));
        if(new_log == NULL) {
            return;
        }
        server->log = new_log;
    }

    server->log[server->log_size-1] = malloc(sizeof(access_log));
    server->log[server->log_size-1]->access_time = time(NULL);
    server->log[server->log_size-1]->type = access_type;
    server->log[server->log_size-1]->user = user;
}

unsigned int log_user(user_data *user) {
    if(user->logged) { // someone is already logged in
        return 0;
    }
    user->logged = 1;

    user->mailbox = malloc(sizeof(t_mailbox));
    user->mailbox->mails = NULL;
    user->mailbox->mail_count = 0;
    user->mailbox->mails_size = 0;
    user->mailbox->deleted_count = 0;

    new_access_log(user, LOGIN_ACCESS);

    return load_mailbox(user);
}

void log_out_user(user_data *user) {
    user->logged = 0;
    if (user->mailbox != NULL) {
        free_mailbox(user->mailbox);
        user->mailbox = NULL;
    }
    new_access_log(user, LOGOUT_ACCESS);
}

user_data *validate_user(char *username, char *password) {
    for(int i = 0; i < server->user_amount; i++) {
        if(strcmp(server->users_list[i]->name, username) == 0 && strcmp(server->users_list[i]->pass, password) == 0) {
            printf("[POP3] Signed in user %s\n", username);
            return server->users_list[i];
        }
    }
    return NULL;
}

unsigned int load_mailbox(user_data *user) {
    char *user_maildir = malloc(PATH_MAX);
    if(!user_maildir) {
        printf("[POP3] Error allocating memory for Maildir.\n");
        return 0;
    }
    snprintf(user_maildir, PATH_MAX, "%s/%s", server->maildir, user->name);

    DIR *dir = opendir(user_maildir);
    if (!dir) {
        free(user_maildir);
        printf("[POP3] No new mails.\n");
        return 0;
    }

    user->mailbox->mails = malloc(MAX_MAILS * sizeof(mail));
    if(!user->mailbox->mails) {
        printf("[POP3] Error allocating memory for mails.\n");
        free(user_maildir);
        closedir(dir);
        return 0;
    }

    char * new_dir = malloc(PATH_MAX);
    if(!new_dir) {
        printf("[POP3] Error allocating memory for 'new' directory.\n");
        free(user_maildir);
        closedir(dir);
        return 0;

    }
    snprintf(new_dir, PATH_MAX, "%s/new", user_maildir);

    DIR *subdir = opendir(new_dir);
    if (!subdir) {
        printf("[POP3] No new mails.\n");
        free(new_dir);
        free(user_maildir);
        closedir(dir);
        return 0;
    }

    struct dirent *entry;
    int count = 0;
    size_t total_size = 0;

    while ((entry = readdir(subdir)) != NULL && count < MAX_MAILS) {
        if (entry->d_type == DT_REG) {
            mail *mail = &user->mailbox->mails[count];
            mail->filename = malloc(PATH_MAX);
            snprintf(mail->filename, PATH_MAX, "%s/%s", new_dir, entry->d_name);

            mail->id = count + 1;
            mail->size = get_file_size(mail->filename);
            mail->deleted = 0;
            mail->fd = -1;

            if (!mail->filename || mail->size == 0) {
                perror("[POP3] Error processing file");
                closedir(subdir);
                closedir(dir);
                free(new_dir);
                free(user_maildir);
                return 0;
            }

            total_size += mail->size;
            count++;
        }
    }

    closedir(subdir);
    closedir(dir);

    user->mailbox->mail_count = count;
    user->mailbox->mails_size = total_size;

    free(new_dir);
    free(user_maildir);
    return 1;
}


size_t get_file_size(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0) {
        return st.st_size;
    } else {
        perror("[POP3] Error getting file size\n");
        return 0;
    }
}

void free_mailbox(t_mailbox* mails) {
    if (mails != NULL) {
        if (mails->mails != NULL) {
            for (int i = 0; i < (mails->mail_count + mails->deleted_count); i++) {
                if (mails->mails[i].filename != NULL) {
                    free(mails->mails[i].filename);
                }
            }
            free(mails->mails);
        }
        free(mails);
    }
}

user_data ** get_users(){
    return server->users_list;
}

size_t get_users_amount(){
    return server->user_amount;
}

unsigned char add_user(char * user_and_pass){
    if(server->user_amount >= MAX_USERS) {
        return FALSE;
    } else {
        if(user(user_and_pass)) {
            return TRUE;
        }
        return FALSE;
    }
}

unsigned char delete_user(char * username){
    for(int i = 0; i < server->user_amount; i++) {
        if(strcmp(server->users_list[i]->name, username) == 0) {
            if (server->users_list[i]->logged) {
                server->users_list[i]->to_delete = TRUE;
                return TRUE;
            }
            free_user_data(server->users_list[i]);
            server->users_list[i] = server->users_list[server->user_amount - 1];
            server->user_amount--;
            return TRUE;
        }
    }
    return FALSE;
}

size_t get_historic_connections() {
    return server->historic_connections;
}


unsigned int get_current_connections() {
    int connections = 0;
    for (int i=0; i < server->user_amount; i++) {
        if (server->users_list[i]->logged) {
            connections++;
        }
    }
    return connections;
}

size_t get_bytes_transferred() {
    return server->bytes_transferred;
}


void add_bytes_transferred(size_t bytes) {
    server->bytes_transferred += bytes;
}



int transform_mail_piping(int mail_fd) {
    if(server->transformation == NULL) {
        set_transformation("/bin/cat");
    }

    int output_pipe[2];
    if(pipe(output_pipe) == -1) {
        printf("[POP3] Error creating output pipes\n");
        return ERR;
    }

    pid_t PID = fork();

    switch (PID) {
        case -1:
            printf("[POP3] Error creating child process\n");
            return ERR;
            break;
        case 0:

            dup2(mail_fd, STDIN_FILENO);
            dup2(output_pipe[1], STDOUT_FILENO);

            close(output_pipe[0]);

            execlp(server->transformation, server->transformation, NULL);

            break;
        default:
            close(output_pipe[1]);
            break;
    }
    
    return output_pipe[0];
}

size_t get_log_size() {
    return server->log_size;
}
access_log ** get_access_log() {
    return server->log;
}

char * get_maildir(){
    return server->maildir;
}
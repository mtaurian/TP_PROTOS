#include "include/pop3.h"
#include "states_definition/include/auth_user.h"
#include "states_definition/include/auth_pass.h"
#include "states_definition/include/transaction.h"
#include "states_definition/include/update.h"
#include <dirent.h>
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
        .on_read_ready    = NULL,
        .on_write_ready   = NULL,
    }
};

void initialize_pop3_server() {
    server = malloc(sizeof(struct pop3_server));
    server->user_amount = 0;
    server->maildir = NULL;
}

void free_pop3_server() {
    for (int i = 0; i < server->user_amount; i++) {
        free_user_data(&server->users_list[i]);
    }
    if(server->maildir) {
        free(server->maildir);
    }
    free(server);
}

void free_user_data(user_data *user) {
    if (user->name) {
        free(user->name);
    }
    if (user->pass) {
        free(user->pass);
    }

    if (user->logged && user->mailbox) {
        free_mailbox(user->mailbox);
        user->mailbox = NULL;
    }
}

void pop3_passive_accept(const struct selector_key *_key) {
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
    clientData->clientFd = client_fd;
    clientData->clientAddress = client_addr;
    clientData->username = NULL;
    clientData->password = NULL;
    clientData->stm.states = states;
    buffer_init(&clientData->clientBuffer, BUFFER_SIZE, clientData->inClientBuffer);
    buffer_init(&clientData->responseBuffer, BUFFER_SIZE, clientData->inResponseBuffer);

    clientData->stm.initial = AUTHORIZATION_USER;
    clientData->stm.max_state = UPDATE;
    stm_init(&clientData->stm);
    ss = selector_register(_key->s, client_fd, &client_handler, OP_READ, clientData);
    if (ss != SELECTOR_SUCCESS) {
        err_msg = "Unable to register client socket handler";
    }


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
        printf("Client connected\n");
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
}

void user(const char *s) {
    user_data *user = &server->users_list[server->user_amount];
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        size_t name_length = strlen(s);
        size_t pass_length = strlen(p);
        char * name = malloc(sizeof(char)*(name_length+1));
        char * pass = malloc(sizeof(char)*(pass_length+1));
        strcpy(name, s);
        strcpy(pass, p);
        user->pass = pass;
        user->name = name;
        user->logged = 0;
        server->user_amount++;
    }
}

void set_maildir(const char *maildir) {
    server->maildir = malloc(PATH_MAX);
    strcpy(server->maildir, maildir);
    printf("MALDIR: %s\n", server->maildir);
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

    return load_mailbox(user);;
}

void log_out_user(user_data *user) {
    user->logged = 0;
    if (user->mailbox != NULL) {
        free_mailbox(user->mailbox);
        user->mailbox = NULL;
    }
}

user_data *validate_user(char *username, char *password) {
    for(int i = 0; i < server->user_amount; i++) {
        if(strcmp(server->users_list[i].name, username) == 0 && strcmp(server->users_list[i].pass, password) == 0) {
            printf("Signed in user %s\n", username); // TODO: do as a log
            return &server->users_list[i]; // TODO: return 1 or 0
        }
    }
    return NULL;
}

unsigned int load_mailbox(user_data *user) {
    char *user_maildir = malloc(PATH_MAX);
    snprintf(user_maildir, PATH_MAX, "%s/%s",server->maildir, user->name);

    DIR *dir = opendir(user_maildir);
    if (!dir) {
        free(user_maildir);
        return 0; // no mails // TODO: error handling
    }

    user->mailbox->mails = malloc(MAX_MAILS * sizeof(mail));

    char *current_dir = malloc(PATH_MAX);
    snprintf(current_dir, PATH_MAX, "%s", user_maildir);

    struct dirent *entry;
    int count = 0;
    size_t total_size = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Ignore `.` y `..`
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            snprintf(current_dir, PATH_MAX, "%s/%s", user_maildir, entry->d_name);
            printf("Direc: %s\n", current_dir);

            DIR *subdir = opendir(current_dir);
            if (!subdir) {
                perror("Error abriendo subdirectorio");
                continue;
            }

            struct dirent *sub_entry;
            while ((sub_entry = readdir(subdir)) != NULL && count < MAX_MAILS) {
                if (sub_entry->d_type == DT_REG) {
                    mail *mail = &user->mailbox->mails[count];
                    mail->filename = malloc(PATH_MAX);
                    snprintf(mail->filename, PATH_MAX, "%s/%s", current_dir, sub_entry->d_name);
                    printf("Direc: %s\n", mail->filename);

                    mail->id = count + 1;
                    mail->size = get_file_size(mail->filename);
                    mail->deleted = 0;
                    mail->fd = -1;

                    if (!mail->filename || mail->size == 0) {
                        closedir(subdir);
                        free(user_maildir);
                        free(current_dir);
                        return 0;
                    }

                    total_size += mail->size;
                    count++;
                }
            }

            closedir(subdir);
        }
    }

    closedir(dir);
    user->mailbox->mail_count = count;
    user->mailbox->mails_size = total_size;
    free(user_maildir);
    free(current_dir);

    return 1;

}

size_t get_file_size(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0) {
        return st.st_size;
    } else {
        perror("Error getting file size\n");  //TODO: manejo de errores
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
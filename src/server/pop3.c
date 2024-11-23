#include "include/pop3.h"
#include "states_definition/include/auth_user.h"
#include "states_definition/include/auth_pass.h"
#include "states_definition/include/transaction.h"

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
        .on_arrival       = NULL,
        .on_departure     = NULL,
        .on_read_ready    = NULL,
        .on_write_ready   = NULL,
    }
};

void initialize_pop3_server() {
    server = malloc(sizeof(struct pop3_server));
    server->user_amount = 0;
}

void pop3_passive_accept(struct selector_key *_key) {
    const char *err_msg = NULL;
    struct sockaddr_storage client_addr;
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
    clientData->stm.states = states;
    buffer_init(&clientData->clientBuffer, BUFFER_SIZE, clientData->inClientBuffer);
    buffer_init(&clientData->responseBuffer, BUFFER_SIZE, clientData->inResponseBuffer);

    clientData->stm.initial = AUTHORIZATION_USER;
    clientData->stm.max_state = UPDATE;
    stm_init(&clientData->stm);
    selector_status ss = selector_register(_key->s, client_fd, &client_handler, OP_READ, clientData);
    if (ss != SELECTOR_SUCCESS) {
       err_msg = "Unable to register client socket handler";
    }
finally:
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
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


void read_handler(struct selector_key *_key) {
    const char *err_msg = NULL;
    client_data *clientData = ATTACHMENT(_key);

    size_t writable_bytes;
    uint8_t *write_ptr = buffer_write_ptr(&clientData->clientBuffer, &writable_bytes);

    // READ from socket into buffer
    ssize_t bytes_received = recv(_key->fd, write_ptr, writable_bytes, 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            err_msg = "Client disconnected";
        } else {
            err_msg = "Error in recv";
        }
        goto leave;
    }

    buffer_write_adv(&clientData->clientBuffer, bytes_received);

    // READ from socket into buffer
    stm_handler_read(&clientData->stm, _key);

    leave:
        if (err_msg) {
            perror(err_msg);
            selector_unregister_fd(_key->s, _key->fd);
            close(_key->fd);
        } else {
            selector_set_interest_key(_key, OP_WRITE);
        }
}

void write_handler(struct selector_key *_key) {
    const char *err_msg = NULL;
    client_data *clientData = ATTACHMENT(_key);

    stm_handler_write(&clientData->stm, _key);

    size_t readable_bytes;
    uint8_t *read_ptr = buffer_read_ptr(&clientData->responseBuffer, &readable_bytes);

    ssize_t bytes_sent = send(_key->fd, read_ptr, readable_bytes, 0);

    if (bytes_sent < 0) {
        err_msg = "Error in send";
        selector_unregister_fd(_key->s, _key->fd);
        close(_key->fd);
        goto leave;
    }

    buffer_read_adv(&clientData->responseBuffer, bytes_sent);

leave:

    if (err_msg) {
        perror(err_msg);
    } else {
        selector_set_interest_key(_key, OP_READ);
    }
}


void user(char *s) {
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

unsigned int log_user(user_data *user) {
    if(user->logged) { // someone is already logged in
        return 0;
    }
    user->logged = 1;
    return 1;
}

void log_out_user(user_data *user) {
    user->logged = 0;
}


void free_pop3_server() {
    for (int i = 0; i < server->user_amount; i++) {
        free(server->users_list[i].name);
        free(server->users_list[i].pass);
    }
    free(server);
}


user_data *validate_user(char *username, char *password) {
    for(int i = 0; i < server->user_amount; i++) {
        if(strcmp(server->users_list[i].name, username) == 0 && strcmp(server->users_list[i].pass, password) == 0) {
            printf("Signed in user %s\n", username); // TODO: do as a log
            return &server->users_list[i];
        }
    }
    return NULL;
}
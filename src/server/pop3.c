#include "include/pop3.h"
#include "states_definition/include/auth_user.h"

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
        .on_arrival       = NULL,
        .on_departure     = NULL,
        .on_read_ready    = NULL,
        .on_write_ready   = auth_user_on_ready_to_write, //todo change
    },
    {
        .state            = TRANSACTION,
        .on_arrival       = NULL,
        .on_departure     = NULL,
        .on_read_ready    = NULL,
        .on_write_ready   = NULL,
    },
    {
        .state            = UPDATE,
        .on_arrival       = NULL,
        .on_departure     = NULL,
        .on_read_ready    = NULL,
        .on_write_ready   = NULL,
    }
};
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
    free(data);
}


void read_handler(struct selector_key *_key) {
    const char *err_msg = NULL;
    client_data *clientData = ATTACHMENT(_key);

    size_t writable_bytes;
    uint8_t *write_ptr = buffer_write_ptr(&clientData->clientBuffer, &writable_bytes);

    // READ from socket into buffer
    stm_handler_read(&clientData->stm, _key);

        if (buffer_can_write(&clientData->clientBuffer))
            selector_set_interest_key(_key, OP_READ | OP_WRITE);

        selector_set_interest_key(_key, OP_WRITE);
}

void write_handler(struct selector_key *_key) {
    const char *err_msg = NULL;
    client_data *clientData = ATTACHMENT(_key);

    stm_handler_write(&clientData->stm, _key);

        if (buffer_can_read(&clientData->clientBuffer))
            selector_set_interest_key(_key, OP_READ | OP_WRITE);

        selector_set_interest_key(_key, OP_READ);
}

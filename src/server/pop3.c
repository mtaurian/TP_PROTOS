#include "include/pop3.h"

static const struct fd_handler client_handler = {
    .handle_read   =  read_handler,
    .handle_write  =  write_handler,
    .handle_close  = close_client,
    .handle_block = NULL,
};

void pop3_passive_accept(struct selector_key *key) {
    const char *err_msg = NULL;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    int client_fd = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
        err_msg = "Error in accept";
        goto finally;
    }

    if (selector_fd_set_nio(client_fd) == -1) {
        err_msg = "Unable to set client socket flags";
        goto finally;
    }
    ClientData* clientData = calloc(1, sizeof(ClientData));
    if (clientData == NULL) {
        return;
    }

    clientData->closed = false;
    clientData->clientFd = client_fd;
    clientData->clientAddress = client_addr;

    buffer_init(&clientData->clientBuffer, BUFFER_SIZE, clientData->inClientBuffer);


    selector_status ss = selector_register(key->s, client_fd, &client_handler, OP_READ, clientData);
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


void close_client(struct selector_key * key) {
    ClientData* data = ATTACHMENT(key);
    if (data->closed)
        return;
    data->closed = true;

    int clientFd = data->clientFd;

    if (clientFd != -1) {
        selector_unregister_fd(key->s, clientFd);
        close(clientFd);
    }
    free(data);
}


void read_handler(struct selector_key *key) {
    const char *err_msg = NULL;
    ClientData *clientData = ATTACHMENT(key);

    size_t writable_bytes;
    uint8_t *write_ptr = buffer_write_ptr(&clientData->clientBuffer, &writable_bytes);

    // READ from socket into buffer
    ssize_t bytes_received = recv(key->fd, write_ptr, writable_bytes, 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            err_msg = "Client disconnected";
        } else {
            err_msg = "Error in recv";
        }
        goto leave;
    }

    buffer_write_adv(&clientData->clientBuffer, bytes_received);

leave:
    if(err_msg) {
        perror(err_msg);
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);

    } else {

        if (buffer_can_write(&clientData->clientBuffer))
            selector_set_interest_key(key,  OP_READ | OP_WRITE);

        selector_set_interest_key(key, OP_WRITE);
    }
}

void write_handler(struct selector_key *key) {
    const char *err_msg = NULL;
    ClientData *clientData = ATTACHMENT(key);
    size_t readable_bytes;
    uint8_t *read_ptr = buffer_read_ptr(&clientData->clientBuffer, &readable_bytes);

    // echo back to client
    ssize_t bytes_sent = send(key->fd, read_ptr, readable_bytes, 0);

    if (bytes_sent < 0) {
        err_msg = "Error in send";
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);
        goto leave;
    }

    buffer_read_adv(&clientData->clientBuffer, bytes_sent);

leave:

    if(err_msg) {
        perror(err_msg);
    } else {
        if (buffer_can_read(&clientData->clientBuffer))
            selector_set_interest_key(key, OP_READ | OP_WRITE);

        selector_set_interest_key(key, OP_READ);
    }
}

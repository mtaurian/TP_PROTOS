#include "../include/pop3.h"
#include "include/utils.h"

void print_response(struct selector_key *key) {
    const char *err_msg = NULL;
    client_data *clientData = ATTACHMENT(key);
    size_t readable_bytes;
    uint8_t *read_ptr = buffer_read_ptr(&clientData->responseBuffer, &readable_bytes);

    ssize_t bytes_sent = send(key->fd, read_ptr, readable_bytes, 0);

    if (bytes_sent < 0) {
        err_msg = "Error in send";
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);
        goto leave;
    }

    buffer_read_adv(&clientData->responseBuffer, bytes_sent);

leave:

    if (err_msg) {
        perror(err_msg);
    } else {
        selector_set_interest_key(key, OP_READ);
    }
}

void fd_to_client_buffer(client_data *clientData, struct selector_key *key) {
    const char *err_msg = NULL;
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
    if (err_msg) {
        perror(err_msg);
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);
    } else {
        if (buffer_can_write(&clientData->clientBuffer))
            selector_set_interest_key(key, OP_READ | OP_WRITE);

        selector_set_interest_key(key, OP_WRITE);
    }
}

void write_std_response(char isOk, char *msg, struct selector_key *key) {
    client_data *clientData = ATTACHMENT(key);
    size_t toWrite;
    if (isOk) {
        buffer_write_string(&clientData->responseBuffer, "-OK\n\0");
    } else {
        buffer_write_string(&clientData->responseBuffer, "-ERR\n\0");
    }
    // buffer_write_ptr(&clientData->responseBuffer, &toWrite);
    // if (toWrite >= 4 && isOk){
    //     buffer_write(&clientData->responseBuffer, '+');
    //     buffer_write(&clientData->responseBuffer, 'O');
    //     buffer_write(&clientData->responseBuffer, 'K');
    //     buffer_write(&clientData->responseBuffer, msg ?  ' ' : '\n');
    // } else if (toWrite >= 5 && !isOk) {
    //     buffer_write(&clientData->responseBuffer, '-');
    //     buffer_write(&clientData->responseBuffer, 'E');
    //     buffer_write(&clientData->responseBuffer, 'R');
    //     buffer_write(&clientData->responseBuffer, 'R');
    //     buffer_write(&clientData->responseBuffer, msg ?  ' ' : '\n');
    // }
    //TODO: Add msg to response
}

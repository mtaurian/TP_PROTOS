#include "include/handlers.h"

#define BUFFER_SIZE 1024


void pop3_passive_accept(struct selector_key *key) {
    printf("Passive accept handler accessed\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    int client_fd = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd < 0) {
        perror("Error when accepting client connection");
        return;
    }

    if (selector_fd_set_nio(client_fd) == -1) {
        perror("Unable to set client socket flags");
        close(client_fd);
        return;
    }

    fd_handler* pop3 = malloc(sizeof(struct fd_handler));

    pop3->handle_read = read_handler;
    pop3->handle_write = write_handler;
    pop3->handle_close = close_client;

    buffer* buffer = malloc(sizeof(struct buffer));
    uint8_t* data = malloc(sizeof(uint8_t) * BUFFER_SIZE);
    buffer_init(buffer, BUFFER_SIZE, data);

    selector_status ss = selector_register(key->s, client_fd, pop3, OP_READ, buffer);
    if (ss != SELECTOR_SUCCESS) {
        perror("Unable to register client socket handler");
        free(data);
        free(buffer);
        close(client_fd);
        return;
    }

    printf("Client connected\n");
}


void close_client(struct selector_key *key) {
    printf("Free buffer handler accessed\n");
    buffer *buffer = key->data;
    free(buffer->data);
    free(buffer);
    selector_destroy(key->s);
    selector_unregister_fd(key->s, key->fd);
    close(key->fd);
}


void read_handler(struct selector_key *key) {
    printf("Read handler accessed\n");

    buffer *buffer = key->data;

    size_t writable_bytes;
    uint8_t *write_ptr = buffer_write_ptr(buffer, &writable_bytes);

    // READ from socket into buffer
    ssize_t bytes_received = recv(key->fd, write_ptr, writable_bytes, 0);
    printf("Bytes received: %ld\n", bytes_received);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Client disconnected.\n");
        } else {
            perror("Error in recv");
        }
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);
        return;
    }

    buffer_write_adv(buffer, bytes_received);

    if (buffer_can_write(buffer))
        selector_set_interest_key(key, OP_NOOP | OP_READ | OP_WRITE);

    selector_set_interest_key(key, OP_WRITE | OP_NOOP);

}

void write_handler(struct selector_key *key) {
    printf("Write handler accessed\n");
    printf("Client fd: %d\n", key->fd);
    buffer *buffer = key->data;


    if (!buffer_can_read(buffer)) {
        selector_set_interest_key(key, OP_READ);
        return;
    }

    size_t readable_bytes;
    uint8_t *read_ptr = buffer_read_ptr(buffer, &readable_bytes);

    // echo back to client
    ssize_t bytes_sent = send(key->fd, read_ptr, readable_bytes, 0);
    printf("Bytes sent: %ld\n", bytes_sent);

    if (bytes_sent < 0) {
        perror("Error in send");
        selector_unregister_fd(key->s, key->fd);
        close(key->fd);
        return;
    }

    buffer_read_adv(buffer, bytes_sent);

    if (buffer_can_read(buffer))
        selector_set_interest_key(key, OP_NOOP | OP_READ | OP_WRITE);

    selector_set_interest_key(key, OP_READ | OP_NOOP);

}

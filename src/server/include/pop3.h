#ifndef HANDLERS_H
#define HANDLERS_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "../../shared/include/selector.h"
#define ATTACHMENT(key) ((ClientData*)(key)->data)
#define BUFFER_SIZE 2048

typedef struct ClientData {

    struct sockaddr_storage clientAddress;
    bool closed;
    int clientFd;

    struct buffer clientBuffer;
    uint8_t inClientBuffer[BUFFER_SIZE];

} ClientData;
void pop3_passive_accept(struct selector_key *key);

void close_client(struct selector_key *key);
void read_handler(struct selector_key *key);
void write_handler(struct selector_key *key);
#endif //HANDLERS_H

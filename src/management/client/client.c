#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../../shared/include/utils.h"
#include "client_args.h"

int conectToServer(struct clientArgs * client_args) {
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;


    struct addrinfo* servAddr;
    int rtnVal = getaddrinfo(client_args->client_addr, client_args->client_port, &addrCriteria, &servAddr);
    if (rtnVal != 0) {
        free(client_args);
        return -1;
    }
    int sock = -1;
    for (struct addrinfo* addr = servAddr; addr != NULL && sock == -1; addr = addr->ai_next) {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock >= 0) {
            errno = 0;
            if (connect(sock, addr->ai_addr, addr->ai_addrlen) != 0) {
                close(sock);
                free(client_args);
                sock = -1;
            }
        }
    }
    freeaddrinfo(servAddr);
    return sock;
}

boolean authenticate(char* usernameAndPassword, int socket) {
    char loginCredentials[BUFFER_SIZE];
    snprintf(loginCredentials, BUFFER_SIZE, "login %s", usernameAndPassword);

    if (send(socket, loginCredentials, strlen(loginCredentials), 0) < 0) {
        printf("[MGMT] Error sending credentials to server\n");
        return false;
    }

    char response[BUFFER_SIZE];
    int bytesRead = read(socket, response, BUFFER_SIZE - 1);
    if (bytesRead <= 0) {
        printf("[MGMT] Error receiving response from server\n");
        return false;
    }

    response[bytesRead] = '\0'; // Null-terminate the response

    if (strncmp(response, "+OK", 3) == 0) {
        return true;
    } else {
        printf("[MGMT] Authentication failed: %s\n", response);
        return false;
    }
}
static int sendStringToSocket(int sock, char* s) {
    if (send(sock, s, strlen(s), 0) <= 0) {
        perror("[MGMT] Error sending message to server\n");
        return -1;
    }
    return 0;
}



boolean send_command(struct clientArgs * client_args, int socket) {
    char commandString[BUFFER_SIZE];
    const char* commandName;
    switch (client_args->command) {
        case USERS:
            return sendStringToSocket(socket, "users");
            break;
        case ADD_USER:
            commandName = "addu";
            snprintf(commandString, BUFFER_SIZE, "%s %s", commandName, client_args->payload);
            return sendStringToSocket(socket, commandString);
            break;
        case DELETE_USER:
            commandName = "deleu";
            snprintf(commandString, BUFFER_SIZE, "%s %s", commandName, client_args->payload);
            return sendStringToSocket(socket, commandString);
            break;
        case METRICS:
            return sendStringToSocket(socket, "metrics");
            break;
        case ACCESS_LOG:
            return sendStringToSocket(socket, "logs");
            break;
        default:
            printf("[MGMT] Invalid command\n");
            return false;
    }
    return true;
}

int main(const int argc,char **argv) {
    struct clientArgs *clientArguments = malloc(sizeof(struct clientArgs));
    parse_args(argc,argv,clientArguments);
    int sock = conectToServer(clientArguments);
    if (sock < 0) {
        perror("[MGMT] socket() failed");
        return -1;
    }
    if(!authenticate(clientArguments->usernameAndPassword, sock) || send_command(clientArguments, sock) ) {
        free(clientArguments);
        close(sock);
        return 0;
    };
    uint8_t c;
    int qty;
    char buffer[3] = {0};

    while ((qty = read(sock, &c, 1)) > 0) {
        putchar(c);
        buffer[0] = buffer[1];
        buffer[1] = buffer[2];
        buffer[2] = c;

        if (buffer[0] == '\n' && buffer[1] == '.' && buffer[2] == '\n') {
            break;
        }
    }
    free(clientArguments);
    close(sock);
    return 0;
}
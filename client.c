#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "src/include/buffer.h"
#include "src/include/selector.h"
#define PORT 1080
#define BUFFER_SIZE 1024

static bool done = false;

static void sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n",signal);
    done = true;
}

int main(const int argc, const char **argv) {
    int client_fd;
    struct sockaddr_in server_addr;

    // creo el socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // asigno ip
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Dirección invalida");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar el socket");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    printf("Conectado al servidor. Escriba un mensaje o presione Ctrl+D para salir.\n");

    buffer* buffer = malloc(sizeof(buffer));
    uint8_t* data = malloc(sizeof(uint8_t) * BUFFER_SIZE);
    buffer_init(buffer, BUFFER_SIZE, data);


    while(!done) {
        if (fgets((char *)buffer->data, BUFFER_SIZE, stdin) == NULL) {
            break; // EOF
        }

        // enviar x socket al servidor
        ssize_t message_length = strlen((char *)buffer->data);
        ssize_t bytes_sent = send(client_fd, buffer->data, message_length, 0);
        if (bytes_sent < 0) {
            perror("Error en send");
            break;
        }

        // recibo rta del servidor
        ssize_t bytes_received = recv(client_fd, buffer->data, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Error en recv");
            break;
        } else if (bytes_received == 0) {
            printf("El servidor cerró la conexión.\n");
            break;
        }


        buffer->data[bytes_received] = '\0';
        printf("%s",(char*)buffer->data);
    }

    close(client_fd);


    free(buffer->data);
    free(buffer);
    return 0;
}

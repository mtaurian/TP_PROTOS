/**
 * main.c - servidor proxy socks concurrente
 *
 * Interpreta los argumentos de línea de comandos, y monta un socket
 * pasivo.
 *
 * Todas las conexiones entrantes se manejarán en éste hilo.
 *
 * Se descargará en otro hilos las operaciones bloqueantes (resolución de
 * DNS utilizando getaddrinfo), pero toda esa complejidad está oculta en
 * el selector.
 */
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

#include "src/include/buffer.h"
#include "src/include/selector.h"

static bool done = false;

#define BUFFER_SIZE 1024

static void close_client(struct selector_key *key) {
    printf("Free buffer handler accessed\n");
    buffer *buffer = key->data;
    free(buffer->data);
    free(buffer);
    close(key->fd);
    selector_unregister_fd(key->s, key->fd);
}


static void read_handler(struct selector_key *key) {
    printf("Read handler accessed\n");

    buffer *buffer = key->data;

    if(!buffer_can_write(buffer)) {
        selector_set_interest_key(key, OP_WRITE);
        return;
    }

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

    /** See if there's space to write in the buffer,
    *  if not, we need to wait for the buffer to be read (write_handler (writes in socket))
   **/
    if (buffer_can_write(buffer)) {
        selector_set_interest_key(key, OP_READ | OP_WRITE);
    } else {
        selector_set_interest_key(key, OP_READ);
    }
}

static void write_handler(struct selector_key *key) {
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

    /** See if theres something to read from the buffer,
     *  if not, we need to wait for new input on buffer (read_handler (reads from socket))
    **/
    if (buffer_can_read(buffer)) {
        selector_set_interest_key(key, OP_READ | OP_WRITE);
    } else {
        selector_set_interest_key(key, OP_READ);
    }

}


static void pop3_passive_accept(struct selector_key *key) {
    printf("Passive accept handler accessed\n");
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
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

    const struct fd_handler pop3 = {
        .handle_read = read_handler,
        .handle_write = write_handler,
        .handle_close = close_client,
    };

    buffer* buffer = malloc(sizeof(struct buffer));
    uint8_t* data = malloc(sizeof(uint8_t) * BUFFER_SIZE);
    buffer_init(buffer, BUFFER_SIZE, data);

    selector_status ss = selector_register(key->s, client_fd, &pop3, OP_READ, buffer);
    if (ss != SELECTOR_SUCCESS) {
        perror("Unable to register client socket handler");
        free(data);
        free(buffer);
        close(client_fd);
        return;
    }

    printf("Client connected\n");
}



static void sigterm_handler(const int signal) {
    printf("Signal %d, cleaning up and exiting\n",signal);
    done = true;
}

int main(const int argc, const char **argv) {
    unsigned port = 1080;

    if(argc == 1) {
        // utilizamos el default
    } else if(argc == 2) {
        char *end     = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end
           || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
           || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
           }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    close(0);

    const char *err_msg = NULL;
    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    // Configurar la dirección del servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);;  // Escuchar en todas las interfaces
    server_addr.sin_port = htons(port);

    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        perror("unable to create socket");
        exit(EXIT_FAILURE); // goto finally
    }

    fprintf(stdout, "Listening on TCP port %d\n", port);

    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    // Asociar el socket a la dirección y puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, 20) < 0) {
        perror("Error en listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }


    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    // selector setup
    if(selector_fd_set_nio(server_fd) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }
    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };

    if(0 != selector_init(&conf)) {
        perror("unable to initialize selector");
        goto finally;
    }

    selector = selector_new(1024);
    if(selector == NULL) {
        perror("unable to create selector");
        goto finally;
    }

    const struct fd_handler pop3 = {
        .handle_read       = pop3_passive_accept,
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    // register as reader
    ss = selector_register(selector, server_fd, &pop3,OP_READ, NULL);
    if(ss != SELECTOR_SUCCESS) {
        printf("unable to register socket handler\n");
        goto finally;
    }

    printf("Servidor escuchando\n");


    for(;!done;) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            perror("Error en selector_select");
            goto finally;
        }
    }

    int ret = 0;
finally:
    /*if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
                                  ss == SELECTOR_IO
                                      ? strerror(errno)
                                      : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }*/
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();

    // socksv5_pool_destroy();

    if(server_fd >= 0) {
        close(server_fd);
        printf("Servidor cerrado.\n");
    }

    return ret;

}




/*
static void
sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n",signal);
    done = true;
}

int
main(const int argc, const char **argv) {
    unsigned port = 1080;

    if(argc == 1) {
        // utilizamos el default
    } else if(argc == 2) {
        char *end     = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end
           || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
           || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // no tenemos nada que leer de stdin
    close(0);

    const char       *err_msg = NULL;
    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);

    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server < 0) {
        err_msg = "unable to create socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d\n", port);

    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(server, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        err_msg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, 20) < 0) {
        err_msg = "unable to listen";
        goto finally;
    }

    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    if(selector_fd_set_nio(server) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }
    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };
    if(0 != selector_init(&conf)) {
        err_msg = "initializing selector";
        goto finally;
    }

    selector = selector_new(1024);
    if(selector == NULL) {
        err_msg = "unable to create selector";
        goto finally;
    }
    const struct fd_handler socksv5 = {
    .handle_read       = socksv5_passive_accept,
    .handle_write      = NULL,
    .handle_close      = NULL, // nada que liberar
    };
    ss = selector_register(selector, server, &socksv5,OP_READ, NULL);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
        goto finally;
    }

    for(;!done;) {
        err_msg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "serving";
            goto finally;
        }
    }
    if(err_msg == NULL) {
        err_msg = "closing";
    }

    int ret = 0;
finally:
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
                                  ss == SELECTOR_IO
                                      ? strerror(errno)
                                      : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();

    // socksv5_pool_destroy();

    if(server >= 0) {
        close(server);
    }
    return ret;
}
*/
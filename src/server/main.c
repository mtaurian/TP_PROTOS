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
#include <arpa/inet.h>

#include "../shared/include/buffer.h"
#include "../shared/include/args.h"
#include "../server/include/pop3.h"

static bool done = false;


static void sigterm_handler(const int signal) {
    printf("Signal %d, cleaning up and exiting\n",signal);
    done = true;
}

/**
 *   Function from https://github.com/ThomasMiz/TornadoProxy/blob/5c72f28677c910f39a089d9ad07814ff77bde873/src/main.c
 *   With mini modifications (log -> perror)
 */
static int setupSockAddr(char* addr, unsigned short port, void* res, socklen_t* socklenResult) {
    int ipv6 = strchr(addr, ':') != NULL;

    if (ipv6) {
        // Parse addr as IPv6
        struct sockaddr_in6 sock6;
        memset(&sock6, 0, sizeof(sock6));

        sock6.sin6_family = AF_INET6;
        sock6.sin6_addr = in6addr_any;
        sock6.sin6_port = htons(port);
        if (inet_pton(AF_INET6, addr, &sock6.sin6_addr) != 1) {
            perror( "Failed IP conversion for IPv6");
            return 1;
        }

        *((struct sockaddr_in6*)res) = sock6;
        *socklenResult = sizeof(struct sockaddr_in6);
        return 0;
    }

    // Parse addr as IPv4
    struct sockaddr_in sock4;
    memset(&sock4, 0, sizeof(sock4));
    sock4.sin_family = AF_INET;
    sock4.sin_addr.s_addr = htonl(INADDR_ANY);
    sock4.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &sock4.sin_addr) != 1) {
        perror("Failed IP conversion for IPv4");
        return 1;
    }

    *((struct sockaddr_in*)res) = sock4;
    *socklenResult = sizeof(struct sockaddr_in);
    return 0;
}

int main(const int argc,char **argv) {
    initialize_pop3_server();
    struct pop3args *pop3config = malloc(sizeof(struct pop3args));

    parse_args(argc,argv,pop3config);
    close(0);
    const char *err_msg       = NULL;

    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int mgmt_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        err_msg = "unable to create socket";
        goto finally;
    }
    if (mgmt_fd < 0) {
        err_msg = "unable to create socket - management";
        goto finally;
    }

    // Configuration of POP3 server
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    if (setupSockAddr(pop3config->pop3_addr, pop3config->pop3_port, &server_addr, &server_addr_len)) {
        err_msg = "Failed to setup server address";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d\n", pop3config->pop3_port);

    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_msg = "unable to bind socket";
        goto finally;
    }

    // Listen to incoming connections
    if (listen(server_fd, 20) < 0) {
        err_msg = "unable to listen";
        goto finally;
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    // selector setup POP3
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
        err_msg = "unable to initialize selector";
        goto finally;
    }

    selector = selector_new(1024);

    if(selector == NULL) {
        err_msg = "unable to create selector";
        goto finally;
    }

    const struct fd_handler pop3 = {
        .handle_read       = pop3_passive_accept,
        .handle_write      = NULL,
        .handle_close      = NULL, // nothing to free
    };

    // register as reader
    ss = selector_register(selector, server_fd, &pop3,OP_READ, NULL);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
        goto finally;
    }

    printf("Server listening\n");

    //MANAGE
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr_len = sizeof(server_addr);

    if (setupSockAddr(pop3config->mng_addr, pop3config->mng_port, &server_addr, &server_addr_len)) {
        err_msg = "Failed to setup server address";
        goto finally;
    }


    // man 7 ip. no importa reportar nada si falla.
    setsockopt(mgmt_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (bind(mgmt_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_msg = "unable to bind socket - management";
        goto finally;
    }

    if (listen(mgmt_fd, 20) < 0) {
        err_msg = "unable to listen - management";
        goto finally;
    }

    if(selector_fd_set_nio(mgmt_fd) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }

    const struct fd_handler management = {
            .handle_read       = NULL, //TODO: set to management_passive_accept
            .handle_write      = NULL,
            .handle_close      = NULL, // nothing to free
    };

    // register as reader
    ss = selector_register(selector, mgmt_fd, &management,OP_NOOP, NULL);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
        goto finally;
    }

    printf("Server listening - Management\n");

    while(!done) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "serving - Management";
            goto finally;
        }
    }

    printf("closing\n");

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

    if(server_fd >= 0) {
        close(server_fd);
        printf("Servidor cerrado.\n");
    }

    if(mgmt_fd >= 0) {
        close(mgmt_fd);
        printf("Servidor de management cerrado.\n");
    }

    free(pop3config);
    free_pop3_server();

    return ret;

}
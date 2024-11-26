#include "client_args.h"
#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

static void
usage(const char *progname) {
    fprintf(stderr,
            "Usage: %s -S <user:password> [OPTION]...\n"
                "\n"
                "   -h                         Imprime la ayuda y termina.\n"
                "   -H <host>                  Direccion donde sirve el servicio de management. Si no se define se usa el valor por defecto 127.0.0.1 \n"
                "   -P <port>                  Puerto donde sirve el servicio de management. Si no se define se usa el valor por defecto 8080.\n"
                "   -U                         Envia una solicitud para obtener los usuarios registrados.\n"
                "   -A <user:password>         Envia una solicitud para agregar un usuario al servidor.\n"
                "   -D <user>                  Envía una solicitud para eliminar un usuario del servidor.\n"
                "   -M                         Envia una solicitud para obtener métricas específicas del servidor.\n"
                "   -L                         Envia una solicitud para obtener los logs del servidor.\n"
                "\n",
       progname);   
    exit(1);
}

void parse_args(const int argc, char **argv, struct clientArgs *args){
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->client_addr = "127.0.0.1";
    args->client_port = "8080";
    args->command = INVALID;

    int c;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { 0,           0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hS:H:P:UA:D:ML", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'S':
                args->usernameAndPassword = optarg;
                break;
            case 'H':
                args->client_addr = optarg;
                break;
            case 'P':
                args->client_port = optarg;
                break;
            case 'U':
                args->command   = USERS;
                break;
            case 'D':
                args->command= DELETE_USER;
                args->payload = optarg;
                break;
            case 'A':
                args->command= ADD_USER;
                args->payload = optarg;
                break;
            case 'M':
                args->command = METRICS;
                break;
            case 'L':
                args->command = ACCESS_LOG;
                break;
            default:
                fprintf(stderr, "[MGMT] Unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (optind < argc) {
        fprintf(stderr, "[MGMT] Argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
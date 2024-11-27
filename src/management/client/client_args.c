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
                "   -h                         Prints help and exits.\n"
                "   -H <host>                  Address where the management service is hosted. If not defined, the default value 127.0.0.1 is used.\n"
                "   -P <port>                  Port where the management service is served. If not defined, the default value 8080 is used.\n"
                "   -U                         Sends a request to get the registered users.\n"
                "   -A <user:password>         Sends a request to add a user to the server.\n"
                "   -D <user>                  Sends a request to delete a user from the server.\n"
                "   -M                         Sends a request to get specific server metrics.\n"
                "   -L                         Sends a request to get the server logs.\n"
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
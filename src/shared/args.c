#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "include/args.h"

#include "../server/manager/include/mgmt.h"
#include "../server/pop3/include/pop3.h"


static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}



static void
version(void) {
    fprintf(stderr, "Version 1\n"
                    "ITBA Protocolos de Comunicación 2024/2 -- Grupo 9\n\n\n"

                    "MIT License\n\n"

                    "Copyright © 2024 Diego Badín, Diego Rabinovich, Magdalena Taurian y Julieta Techenski\n\n"
                    
                    "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated\n"
                    "documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation\n"
                    "the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and\n"
                    "to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\n"
                    
                    "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\n"
                    
                    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING\n"
                    "BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
                    "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,\n"
                    "DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
                    "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
            );
}

/*
Estructura de la carpeta para el argumento -d:
$ tree .
.
└── user1
    ├── cur
    ├── new
    │   └── mail1
    └── tmp
*/
static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h                         Imprime la ayuda y termina.\n"
        "   -l <POP3 addr>             Dirección donde servirá el servidor POP3.\n"
        "   -L <MGMT addr>             Dirección donde servirá el servicio de management.\n"
        "   -p <POP3 port>             Puerto entrante conexiones POP3.\n"
        "   -P <MGMT port>             Puerto entrante conexiones configuracion\n"
        "   -u <user>:<password>       Usuario y contraseña de usuario que puede usar el servidor.\n"
        "   -U <user>:<password>       Usuario y contraseña de usuario admin que puede usar el servicio de management.\n"
        "   -v                         Imprime información sobre la versión versión y termina.\n"
        "   -d <path>                  Carpeta donde residen los Maildirs.\n"
        "   -t <cmd>                   Comando para aplicar transformaciones"
        "\n",
        progname);
    exit(1);
}

void parse_args(const int argc, char **argv, struct pop3args *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->pop3_addr = "0.0.0.0";
    args->pop3_port = 1080;

    args->mng_addr   = "127.0.0.1";
    args->mng_port   = 8080;


    int c;
    int nusers = 0;
    int nadmins = 0;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { 0,           0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hl:L:p:P:u:vd:t:U:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'l':
                args->pop3_addr = optarg;
                break;
            case 'L':
                args->mng_addr = optarg;
                break;
            case 'p':
                args->pop3_port = port(optarg);
                break;
            case 'P':
                args->mng_port   = port(optarg);
                break;
            case 'u':
                if(nusers >= MAX_USERS) {
                    fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                    exit(1);
                } else {
                    if(user(optarg)) nusers++;
                }
                break;
            case 'v':
                version();
                exit(0);
                break;
            case 'd':
                set_maildir(optarg);
                break;
            case 't':
                set_transformation(optarg);
                break;
            case 'U':
                if(nadmins >= MAX_MGMT_USERS) {
                    fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                    exit(1);
                } else {
                    mgmt_user(optarg);
                    nadmins++;
                }
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}

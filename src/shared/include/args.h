#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>

#define MAX_USERS 10

struct users {
    char *name;
    char *pass;
};


struct pop3args {
    char           *pop3_addr;
    unsigned short  pop3_port;

    char           *mng_addr;
    unsigned short  mng_port;


    struct users    users[MAX_USERS];
    char           *maildir;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char **argv, struct pop3args *args);

#endif

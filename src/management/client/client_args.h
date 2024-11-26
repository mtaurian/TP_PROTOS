#ifndef CLIENT_ARGS_H
#define CLIENT_ARGS_H

#include <stdbool.h>
#include "../../shared/include/utils.h"

struct clientArgs {
 char * client_addr;
 char * client_port;
 commands command;
 char * payload;
 char *usernameAndPassword;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void parse_args(const int argc, char **argv, struct clientArgs *args);

#endif //CLIENT_ARGS_H

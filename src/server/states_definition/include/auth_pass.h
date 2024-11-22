#ifndef TP_PROTOS_AUTH_PASS_H
#define TP_PROTOS_AUTH_PASS_H

#include "../../../shared/include/selector.h"
#include "../../include/pop3.h"
#include "../../include/handlers.h"
#include "utils.h"
#include "../../../shared/include/parser_utils.h"

enum auth_pass_commands {
    PASS = 0, QUIT_PASS
};

typedef struct auth_pass_request {
    enum auth_pass_commands command;
    char * payload;
} auth_pass_request;

void auth_pass_on_arrival(unsigned state, struct selector_key *key);
void auth_pass_on_departure(unsigned state, struct selector_key *key);
unsigned int auth_pass_on_ready_to_read(struct selector_key *key);
unsigned int auth_pass_on_ready_to_write(struct selector_key *key);

#endif //TP_PROTOS_AUTH_PASS_H

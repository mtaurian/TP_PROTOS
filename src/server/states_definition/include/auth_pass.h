#ifndef TP_PROTOS_AUTH_PASS_H
#define TP_PROTOS_AUTH_PASS_H

#include "../../../shared/include/selector.h"
#include "../../include/pop3.h"
#include "../../include/handlers.h"
#include "utils.h"
#include "../../../shared/include/parser_utils.h"

void auth_pass_on_arrival(unsigned state, struct selector_key *key);
void auth_pass_on_departure(unsigned state, struct selector_key *key);
unsigned int auth_pass_on_ready_to_read(struct selector_key *key);
unsigned int auth_pass_on_ready_to_write(struct selector_key *key);

#endif //TP_PROTOS_AUTH_PASS_H

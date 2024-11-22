#ifndef TP_PROTOS_AUTH_USER_H
#define TP_PROTOS_AUTH_USER_H

#include "../../../shared/include/selector.h"
#include "utils.h"

void auth_user_on_arrival(unsigned state, struct selector_key *key);
void auth_user_on_departure(unsigned state, struct selector_key *key);
unsigned int auth_user_on_ready_to_read(struct selector_key *key);
unsigned int auth_user_on_ready_to_write(struct selector_key *key);

#endif //TP_PROTOS_AUTH_USER_H

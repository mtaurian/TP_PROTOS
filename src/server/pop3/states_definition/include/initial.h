#ifndef TP_PROTOS_INITIAL_H
#define TP_PROTOS_INITIAL_H

#include "../../include/pop3.h"
#include "../../../../shared/include/utils.h"

void initial_on_arrival(unsigned state, struct selector_key *key);
void initial_on_departure(unsigned state, struct selector_key *key);
unsigned int initial_on_ready_to_write(struct selector_key *key);


#endif //TP_PROTOS_INITIAL_H
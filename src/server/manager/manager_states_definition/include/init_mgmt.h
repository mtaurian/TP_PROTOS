#ifndef TP_PROTOS_INIT_MGMT_H
#define TP_PROTOS_INIT_MGMT_H

#include "../../../../shared/include/utils.h"

void init_on_arrival(unsigned state, struct selector_key *key);
void init_on_departure(unsigned state, struct selector_key *key);
unsigned int init_on_ready_to_write(struct selector_key *key);


#endif //TP_PROTOS_INIT_MGMT_H
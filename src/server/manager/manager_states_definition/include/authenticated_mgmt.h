#ifndef TP_PROTOS_AUTHENTICATED_MGMT_H
#define TP_PROTOS_AUTHENTICATED_MGMT_H
#include "../../../../shared/include/selector.h"
#include "../../../../shared/include/utils.h"
#include "../../include/mgmt_handlers.h"
#include <stdio.h>

void authenticated_on_arrival(unsigned state, struct selector_key *key);
void authenticated_on_departure(unsigned state, struct selector_key *key);
unsigned int authenticated_on_read_ready(struct selector_key *key);
unsigned int authenticated_on_write_ready(struct selector_key *key);

#endif //TP_PROTOS_AUTHENTICATED_MGMT_H

#ifndef TP_PROTOS_NON_AUTHENTICATED_MGMT_H
#define TP_PROTOS_NON_AUTHENTICATED_MGMT_H
#include <stdio.h>
#include "../../../../shared/include/selector.h"
#include "../../../../shared/include/utils.h"
#include "../../include/mgmt.h"

void non_authenticated_on_arrival(unsigned state, struct selector_key *key);
unsigned int non_authenticated_on_read_ready(struct selector_key *key);
void non_authenticated_on_departure(const unsigned state, struct selector_key *key);
unsigned int non_authenticated_on_write_ready(struct selector_key *key);

#endif //TP_PROTOS_NON_AUTHENTICATED_MGMT_H

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "../../../shared/include/selector.h"

void transaction_on_arrival(unsigned state, struct selector_key *key);
void transaction_on_departure(unsigned state, struct selector_key *key);
unsigned int transaction_on_ready_to_read(struct selector_key *key);
unsigned int transaction_on_ready_to_write(struct selector_key *key);

#endif //TRANSACTION_H

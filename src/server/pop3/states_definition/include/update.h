#ifndef UPDATE_H
#define UPDATE_H
#include "../../include/handlers.h"

void update_on_arrival(unsigned state, struct selector_key *key);
unsigned int update_on_ready_to_read(struct selector_key *key);
unsigned int update_on_ready_to_write(struct selector_key *key);

#endif //UPDATE_H

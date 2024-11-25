#include "include/non_authenticated_mgmt.h"

void non_authenticated_on_arrival(const unsigned state, struct selector_key *key) {
    printf("Entered in NON_AUTHENTICATED state\n");
}

void non_authenticated_on_departure(const unsigned state, struct selector_key *key) {
    printf("Exited NON_AUTHENTICATED state\n");
}

unsigned int non_authenticated_on_read_ready(struct selector_key *key){
    return 0;
}

unsigned int non_authenticated_on_write_ready(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

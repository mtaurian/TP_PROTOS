#include "include/init_mgmt.h"

void init_on_arrival(unsigned state, struct selector_key *key){
    printf("[MGMT] Entered in INIT state\n");
}

void init_on_departure(unsigned state, struct selector_key *key){
    printf("[MGMT] Exited INIT state\n");
}

unsigned int init_on_ready_to_write(struct selector_key *key){
    write_ok_message(key, INIT_BANNER);
    return NON_AUTHENTICATED;
}

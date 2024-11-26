#include "include/initial.h"

void initial_on_arrival(unsigned state, struct selector_key *key){
    printf("[POP3] Entered in INITIAL state\n");
}

void initial_on_departure(unsigned state, struct selector_key *key){
    printf("[POP3] Exited INITIAL state\n");
}

unsigned int initial_on_ready_to_write(struct selector_key *key){
    write_ok_message(key, INITIAL_BANNER);
    return AUTHORIZATION_USER;
}

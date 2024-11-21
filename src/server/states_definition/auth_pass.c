#include "./include/auth_pass.h"


void auth_pass_on_arrival(unsigned state, struct selector_key *key){

}

void auth_pass_on_departure(unsigned state, struct selector_key *key){

}

unsigned int auth_pass_on_ready_to_read(struct selector_key *key){
    return 0;
}

unsigned int auth_pass_on_ready_to_write(struct selector_key *key){
    return 0;
}

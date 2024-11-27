#include "include/update.h"


void update_on_arrival(unsigned state, struct selector_key *key){
    printf("[POP3] Entered in UPDATE state\n");
    handle_update_quit(key);
}

unsigned int update_on_ready_to_read(struct selector_key *key){
    client_data *clientData = ATTACHMENT(key);
    if(clientData->readyToLogout && !buffer_can_read(&clientData->responseBuffer)){
        close_client(key);
    }
    return UPDATE;
}

unsigned int update_on_ready_to_write(struct selector_key *key){
    return UPDATE;
}
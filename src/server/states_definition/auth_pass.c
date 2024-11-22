#include "./include/auth_pass.h"

void auth_pass_on_arrival(unsigned state, struct selector_key *key){
    printf("Entered in AUTH_PASS state\n");
}

void auth_pass_on_departure(unsigned state, struct selector_key *key){
    printf("Exited AUTH_PASS state\n");
}

unsigned int auth_pass_on_ready_to_read(struct selector_key *key){
    user_request * entry = parse(key, AUTHORIZATION_PASSWORD);
    char * message = NULL;
    int ret = AUTHORIZATION_PASSWORD;

    if(entry == NULL){
        message =  "Unknown command.\n";
        write_std_response(0, message, key);
        return ret;
    }

    switch (entry->command) {
        case PASS:
            if(handle_pass(key, entry->arg)){
                ret = TRANSACTION;
                message =  "Authentication successful\n";
                write_std_response(1, message, key);
            } else { // passwords don't match
                message =  "Authentication failed\n";
                write_std_response(0, message, key);
                ret = AUTHORIZATION_USER;
            }
        break;
        case QUIT:
            handle_quit(key);
            message =  "Goodbye\n";
            write_std_response(1,message, key);
        	break;
        default:
            message =  "Authentication needed to run command.\n";
            write_std_response(0, NULL, key);
        	break;
    }

    free(entry->arg);
    free(entry);

    return ret;
}

unsigned int auth_pass_on_ready_to_write(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

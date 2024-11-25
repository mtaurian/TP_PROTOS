#include "./include/auth_pass.h"

void auth_pass_on_arrival(unsigned state, struct selector_key *key){
    printf("[POP3] Entered in AUTH_PASS state\n");
}

void auth_pass_on_departure(unsigned state, struct selector_key *key){
    printf("[POP3] Exited AUTH_PASS state\n");
}

unsigned int auth_pass_on_ready_to_read(struct selector_key *key){
    user_request * entry = parse(key);
    char * message = NULL;
    int ret = AUTHORIZATION_PASSWORD;

    if(entry->command == INVALID){
        write_error_message(key, UNKNOWN_COMMAND);
        free(entry);
        return ret;
    }

    switch (entry->command) {
        case PASS:
            if(handle_pass(key, entry->arg)){
                ret = TRANSACTION;
                message =  "Authentication successful\n";
                write_std_response(OK, message, key);
            } else { // passwords don't match
                write_error_message(key, AUTHENTICATION_FAILED);
                ret = AUTHORIZATION_USER;
            }
            break;
        case QUIT:
            free(entry);
            handle_quit(key);
        	break;
        default:
            write_error_message(key, UNKNOWN_COMMAND);
        	break;
    }

    free(entry);

    return ret;
}

unsigned int auth_pass_on_ready_to_write(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

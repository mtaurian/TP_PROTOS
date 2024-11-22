#include "include/auth_user.h"


void auth_user_on_arrival(const unsigned state, struct selector_key *key){
    printf("Entered in AUTH_USER state\n");
}

void auth_user_on_departure(const unsigned state, struct selector_key *key){
    printf("Exited AUTH_USER state\n");
}

unsigned int auth_user_on_ready_to_read(struct selector_key *key){
    client_data *clientData = ATTACHMENT(key);
    user_request * entry = parse(key, AUTHORIZATION_USER);

    char * message = NULL;
    int ret = AUTHORIZATION_USER;

    if(entry == NULL){
        message =  "Unknown command.\n";
        write_std_response(0, message, key);
        return ret;
    }

    switch (entry->command) {
        case USER:
            if(handle_user(key, entry->arg)){
                ret = AUTHORIZATION_PASSWORD;
                write_std_response(1,NULL, key);
            } else { // not a valid user
                message =  "Authentication failed\n";
                write_std_response(0,message, key);
            }

            break;
        case QUIT:
            handle_quit(key);
            message =  "Goodbye\n";
            write_std_response(1,message, key);
            break;
        case PASS:
            message =  "No username given.\n";
            write_std_response(0,message, key);
        default:
            message =  "Authentication needed to run command.\n";
            write_std_response(0, NULL, key);
            break;
    }

    free(entry->arg);
    free(entry);

    return ret;
}

unsigned int auth_user_on_ready_to_write(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}



#include "include/auth_user.h"


void auth_user_on_arrival(const unsigned state, struct selector_key *key){
    printf("[POP3] Entered in AUTH_USER state\n");
}

void auth_user_on_departure(const unsigned state, struct selector_key *key){
    printf("[POP3] Exited AUTH_USER state\n");
}

unsigned int auth_user_on_ready_to_read(struct selector_key *key){
    client_data *clientData = ATTACHMENT(key);
    user_request entry = parse(key);

    int ret = AUTHORIZATION_USER;

    if(entry.command == INVALID){
        write_error_message(key, UNKNOWN_COMMAND);
        return ret;
    }

    switch (entry.command) {
        case USER:
            if(handle_user(key, entry.arg)){
                ret = AUTHORIZATION_PASSWORD;
                write_ok_message(key, JUST_OK);
            } else { // not a valid user
                write_error_message(key, AUTHENTICATION_FAILED);
            }
            break;
        case QUIT:
            write_ok_message(key, LOGOUT_OUT);
            handle_quit(key);
            break;
        case PASS:
            write_error_message(key, NO_USERNAME_GIVEN);
            break;
        default:
            write_error_message(key, UNKNOWN_COMMAND);
            break;
    }


    return ret;
}

unsigned int auth_user_on_ready_to_write(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}



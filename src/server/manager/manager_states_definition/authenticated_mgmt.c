#include "include/authenticated_mgmt.h"

void authenticated_on_arrival(const unsigned state, struct selector_key *key) {
    printf("Entered in AUTHENTICATED state\n");
}

void authenticated_on_departure(const unsigned state, struct selector_key *key) {
    printf("Exited AUTHENTICATED state\n");
}

unsigned int authenticated_on_read_ready(struct selector_key *key){
    user_request entry = parse(key);
    int ret = AUTHENTICATED;
    if (entry.command == INVALID) {
        write_error_message(key, UNKNOWN_COMMAND);
        return ret;
    }

    switch (entry.command) {
        case USERS:
             handle_users(key);
            break;
        case ADD_USER:

            break;
        case DELETE_USER:

            break;
        case METRICS:

            break;
        case ACCESS_LOG:

            break;
        default:
            write_error_message(key, UNKNOWN_COMMAND);
            break;
    }
    return ret;
}

unsigned int authenticated_on_write_ready(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

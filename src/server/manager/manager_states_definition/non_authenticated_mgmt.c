#include "include/non_authenticated_mgmt.h"
#include "../../pop3/include/handlers.h"
#include "../mgmt_handlers.h"

void non_authenticated_on_arrival(const unsigned state, struct selector_key *key) {
    printf("Entered in NON_AUTHENTICATED state\n");
}

void non_authenticated_on_departure(const unsigned state, struct selector_key *key) {
    printf("Exited NON_AUTHENTICATED state\n");
}

unsigned int non_authenticated_on_read_ready(struct selector_key *key) {
    user_request *entry = parse(key);
    int ret = NON_AUTHENTICATED;
    if (entry == NULL || entry->command == INVALID) {
        write_error_message(key, UNKNOWN_COMMAND);
        return ret;
    }

    switch (entry->command) {
        case LOGIN:
            if (handle_login(key, entry->arg)) {
                write_std_response(OK, "Logged in", key);
                ret = AUTHENTICATED;
            } else {
                write_error_message(key, AUTHENTICATION_FAILED);
            }
            break;
        case QUIT:
            handle_quit(key);
            break;
        default:
            write_error_message(key, UNKNOWN_COMMAND);
            break;
    }
    return ret;
}

unsigned int non_authenticated_on_write_ready(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

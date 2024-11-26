#include "include/authenticated_mgmt.h"

void authenticated_on_arrival(const unsigned state, struct selector_key *key) {
    printf("[MGMT] Entered in AUTHENTICATED state\n");
}

void authenticated_on_departure(const unsigned state, struct selector_key *key) {
    printf("[MGMT] Exited AUTHENTICATED state\n");
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
             if (!handle_users(key)){
                 write_error_message(key, INTERNAL_ERROR);
             }
            break;
        case ADD_USER:
            handle_add_user(key, entry.arg);
            break;
        case DELETE_USER:
            if (handle_delete_user(key, entry.arg)){
                write_ok_message(key, USER_DELETED);
            } else {
                write_error_message(key, CANNOT_DEL_USER);
            }
            break;
        case METRICS:
            if (!handle_metrics(key)){
                write_error_message(key, INTERNAL_ERROR);
            }
            break;
        case ACCESS_LOG:
            if (!handle_access_log(key)){
                write_error_message(key, INTERNAL_ERROR);
            }
            break;
        default:
            write_error_message(key, UNKNOWN_COMMAND);
            break;
    }
    buffer_write_string(&ATTACHMENT(key)->responseBuffer, ".\n");
    return ret;
}

unsigned int authenticated_on_write_ready(struct selector_key *key){
    return ATTACHMENT(key)->stm.current->state;
}

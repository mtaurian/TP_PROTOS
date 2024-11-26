#include "include/mgmt_handlers.h"

#include <time.h>

boolean handle_login(struct selector_key *key, char *arg){
    client_data *clientData = ATTACHMENT(key);
    char * username = strtok(arg, ":");
    char * password = strtok(NULL, ":");
    super_user_data * spu = validate_admin(username, password);

    if (spu == NULL) return FALSE;

    clientData->super_user = spu;
    return true;
}

boolean handle_users(struct selector_key *key) {
    client_data *clientData = ATTACHMENT(key);
    user_data **users = get_users();
    if (users == NULL) {
        return FALSE;
    }

    size_t users_amount = get_users_amount();
    size_t response_size = 20 + users_amount * 50; // Estimación del tamaño necesario
    char *response = malloc(response_size);
    if (response == NULL) {
        return FALSE;
    }

    snprintf(response, response_size, "Users quantity: %zu\r\n", users_amount);

    for (int i = 0; i < users_amount; i++) {
        snprintf(response + strlen(response), response_size - strlen(response), "%d -> %-20s %s\n", i, users[i]->name, users[i]->logged ? "online" : "offline");
    }

    write_std_response(OK, response, key);
    free(response);
    return TRUE;
}

boolean handle_add_user(struct selector_key * key, char * arg){
    if (add_user(arg)) {
        write_ok_message(key, USER_ADDED);
        return TRUE;
    } else {
        write_error_message(key, CANNOT_ADD_USER);
        return FALSE;
    }
}

boolean handle_delete_user(struct selector_key * key, char * arg){
    return delete_user(arg);
}

boolean handle_metrics(struct selector_key * key){
    unsigned int current_connections = get_current_connections();
    size_t bytes_transferred = get_bytes_transferred();
    unsigned int historic_connections = get_historic_connections();

    char * metrics = malloc(MAX_RESPONSE);
    if (metrics == NULL) {
        return FALSE;
    }

    snprintf(metrics, MAX_RESPONSE, "\nCurrent connections: %u\nBytes transferred: %zu\nHistoric connections: %u\r\n", current_connections, bytes_transferred, historic_connections);

    write_std_response(OK, metrics, key);
    free(metrics);
    return TRUE;
}


unsigned int handle_access_log(struct selector_key *key) {
    access_log **log = get_access_log();
    size_t log_entry_size = MAX_RESPONSE;
    char *log_entry = malloc(log_entry_size);
    if (log_entry == NULL) {
        return FALSE;
    }

    for(int i = 0; i < get_log_size(); i++) {
        size_t required_size = strlen(log_entry) + MAX_RESPONSE;
        if (required_size > log_entry_size) {
            log_entry_size = required_size;
            char *temp = realloc(log_entry, log_entry_size);
            if (temp == NULL) {
                free(log_entry);
                return FALSE;
            }
            log_entry = temp;
        }

        strftime(log_entry, MAX_RESPONSE, "%Y-%m-%d %H:%M:%S", localtime(&log[i]->access_time));
        snprintf(log_entry, MAX_RESPONSE, " %s %s\n", log[i]->user->name, log[i]->type ? "LOGIN" : "LOGOUT");
    }
    write_std_response(OK, log_entry, key);
    free(log_entry);
    return TRUE;
}
#include "include/mgmt_handlers.h"

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
    user_data *users = get_users();
    if (users == NULL) {
        write_error_message(key, INTERNAL_ERROR);
        return FALSE;
    }

    size_t users_amount = get_users_amount();
    size_t response_size = 20 + users_amount * 50; // Estimación del tamaño necesario
    char *response = malloc(response_size);
    if (response == NULL) {
        write_error_message(key, INTERNAL_ERROR);
        return FALSE;
    }

    snprintf(response, response_size, "Users qty: %zu\n", users_amount);

    for (int i = 0; i < users_amount; i++) {
        snprintf(response + strlen(response), response_size - strlen(response), "%d -> %-20s %s\n", i, users[i].name, users[i].logged ? "online" : "offline");
    }

    write_std_response(OK, response, key);
    free(response);
    return TRUE;
}
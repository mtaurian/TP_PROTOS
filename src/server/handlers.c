#include "include/handlers.h"
#include "include/pop3.h"
#include "states_definition/include/utils.h"

int handle_user(struct selector_key *_key, char * username){
    client_data * clientData = ATTACHMENT(_key);
    unsigned long userNameLength = strlen(username);
    clientData->username = malloc((userNameLength+1) * sizeof(char));

    strcpy(clientData->username, username);

    write_std_response(1, NULL, _key);

    printf("Username set: %s\n", username);
    return 1;
}

void handle_quit(struct selector_key *key){
    close_client(key);
}

int handle_pass(struct selector_key *_key, char * password){
    client_data * clientData = ATTACHMENT(_key);
    unsigned long length = strlen(password);
    clientData->password = malloc((length+1) * sizeof(char));

    strcpy(clientData->password, password);

    write_std_response(1, NULL, _key);

    printf("Password: %s\n", password);  //TODO: Remove this print
    return 1;
}
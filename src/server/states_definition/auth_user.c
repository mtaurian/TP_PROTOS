
#include "include/auth_user.h"
#include "../include/handlers.h"
#include "include/utils.h"
#include "../../shared/include/parser_utils.h"
#include "../include/pop3.h"

static auth_user_request * parse(struct selector_key * key){
    client_data * clientData= ATTACHMENT(key);

    fd_to_client_buffer(clientData, key);

    uint8_t entry = buffer_read(&clientData->clientBuffer);

    //TODO: Change all this command compares to parse_utils :)
    char match = 1;
    char maybeUser = 1;
    char maybeQuit = 1;
    const char * userCmd = "USER";
    const char * quitCmd = "QUIT";
    while(match && entry != ' ' && entry) {
        if (maybeUser && (entry == *userCmd || entry == *userCmd + 32)) {
            userCmd++;
            maybeQuit = 0;
        } else if (maybeQuit && (entry == *quitCmd || entry == *quitCmd + 32)) {
            quitCmd++;
            maybeUser = 0;
        } else {
            match = 0;
        }
            entry = buffer_read(&clientData->clientBuffer);
    }

    auth_user_request *request = malloc(sizeof(auth_user_request));

    if(match){
        request->command = maybeQuit ? QUIT_USER : USER;
    }

    size_t toRead;
    buffer_read_ptr(&clientData->clientBuffer, &toRead);

    entry = buffer_read(&clientData->clientBuffer);
    request->payload = malloc((toRead+1) * sizeof(uint8_t));
    int i;
    for(i = 0; entry && i < toRead; i++){
        request->payload[i] = (char) entry;
        entry = buffer_read(&clientData->clientBuffer);
    }
    request->payload[i] = '\0';
    return request;
}

void auth_user_on_arrival(const unsigned state, struct selector_key *key){
    printf("Entered in AUTH_USER state\n");
}

void auth_user_on_departure(const unsigned state, struct selector_key *key){
    printf("Exited AUTH_USER state\n");
}

unsigned int auth_user_on_ready_to_read(struct selector_key *key){
    auth_user_request * entry = parse(key);
    char * message = malloc(100 * sizeof(char));
    int ret = AUTHORIZATION_USER;

    switch (entry->command) {
        case USER:
            if(handle_user(key, entry->payload)){
                ret = AUTHORIZATION_PASSWORD;
                strcpy(message,  "User accepted\n");
                write_std_response(1,message, key);
            } else { // not a valid user
                strcpy(message,  "Authentication failed\n");
                write_std_response(0,message, key);
            }
            break;
        case QUIT_USER:
            handle_quit(key);
            strcpy(message, "Goodbye\n");
            write_std_response(1,message, key);
            break;
        default:
            write_std_response(0, NULL, key);
            break;
    }

    free(entry->payload);
    free(entry);

    free(message);

    return ret;
}

unsigned int auth_user_on_ready_to_write(struct selector_key *key){
    print_response(key);
    return ATTACHMENT(key)->stm.current->state;
}



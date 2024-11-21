#include "./include/auth_pass.h"
#include "../include/handlers.h"
#include "include/utils.h"
#include "../../shared/include/parser_utils.h"
#include "../include/pop3.h"

static auth_pass_request * parse(struct selector_key * key){
    client_data * clientData= ATTACHMENT(key);

    fd_to_client_buffer(clientData, key);

    uint8_t entry = buffer_read(&clientData->clientBuffer);

    //TODO: Change all this command compares to parse_utils :)
    char match = 1;
    char maybePass = 1;
    char maybeQuit = 1;
    const char * userCmd = "PASS";
    const char * quitCmd = "QUIT";
    while(match && entry != ' ' && entry) {
        if (maybePass && (entry == *userCmd || entry == *userCmd + 32)) {
            userCmd++;
            maybeQuit = 0;
        } else if (maybeQuit && (entry == *quitCmd || entry == *quitCmd + 32)) {
            quitCmd++;
            maybePass = 0;
        } else {
            match = 0;
        }
        entry = buffer_read(&clientData->clientBuffer);
    }

    auth_pass_request *request = malloc(sizeof(auth_pass_request));

    if(match){
        request->command = maybeQuit ? QUIT_PASS : PASS;
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


void auth_pass_on_arrival(unsigned state, struct selector_key *key){
    printf("Entered in AUTH_PASS state\n");
}

void auth_pass_on_departure(unsigned state, struct selector_key *key){
    printf("Exited AUTH_PASS state\n");
}

unsigned int auth_pass_on_ready_to_read(struct selector_key *key){
    auth_pass_request * entry = parse(key);
    char * message = NULL;
    int ret = AUTHORIZATION_PASSWORD;

    switch (entry->command) {
        case PASS:
            if(handle_pass(key, entry->payload)){
                ret = TRANSACTION;
                message =  "Authentication successful\n";
                write_std_response(1, message, key);
            } else { // passwords don't match
                message =  "Authentication failed\n";
                write_std_response(0, message, key);
                ret = AUTHORIZATION_USER;
            }
        break;
        case QUIT_PASS:
            handle_quit(key);
            message =  "Goodbye\n";
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

unsigned int auth_pass_on_ready_to_write(struct selector_key *key){
    return 0;
}

#include "../include/pop3.h"
#include "include/utils.h"

void write_std_response(char isOk, char *msg, struct selector_key *key) {
    client_data *clientData = ATTACHMENT(key);
    size_t toWrite;

    if (isOk) {
        buffer_write_string(&clientData->responseBuffer, "+OK ");
    } else {
        buffer_write_string(&clientData->responseBuffer, "-ERR ");
    }

    if(msg){
        buffer_write_string(&clientData->responseBuffer, msg);
    } else {
    	buffer_write_string(&clientData->responseBuffer, "\n");
    }
}



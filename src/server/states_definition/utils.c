#include "include/utils.h"

struct command_struct all_commands[COMMAND_AMOUNT] = {
    { .string = "user", .command = USER, .has_params = TRUE },
    { .string = "pass", .command = PASS, .has_params = TRUE  },
    { .string = "stat", .command = STAT, .has_params = FALSE  },
    { .string = "list", .command = LIST, .has_params = FALSE  }, // list can have argument
    { .string = "retr", .command = RETR, .has_params = TRUE  },
    { .string = "dele", .command = DELE, .has_params = TRUE  },
    { .string = "noop", .command = NOOP, .has_params = FALSE  },
    { .string = "rset", .command = RSET, .has_params = FALSE  },
    { .string = "quit", .command = QUIT, .has_params = FALSE  },
    { .string = ""     ,.command = INVALID, .has_params = FALSE}
};




// Función para buscar un comando en la lista de comandos permitidos
commands findCommand(char *command) {
    for (int i = 0; i < COMMAND_AMOUNT; i++) {
        if (strcmp(command, all_commands[i].string) == 0) {
            return all_commands[i].command;
        }
    }
    return INVALID;
}


user_request * parse(struct selector_key * key) {
    user_request * result = calloc(1,sizeof(user_request));
    struct client_data * client_Data = ATTACHMENT(key);
    if(!buffer_can_read(&client_Data->clientBuffer)){
        return result;
    }

    size_t readable_bytes;
    uint8_t * read_ptr = buffer_read_ptr(&client_Data->clientBuffer, &readable_bytes);
    char *token = strtok((char * ) read_ptr, " \r\n");

    if (token == NULL || token[0]=='\r' || token[0]=='\n') {
        buffer_read_adv(&client_Data->clientBuffer, readable_bytes);
        return result;
    }

    result->command = findCommand(token);

    if (result->command  == INVALID) {
        buffer_read_adv(&client_Data->clientBuffer, readable_bytes);
        return result;
    }

    if (all_commands[result->command].has_params || result->command == LIST) {
        result->is_valid = true;
        if((token = strtok(NULL, "\r\n")) != NULL) {
            strcpy(result->arg, token);
        }
        if (result->arg[0] =='\0' && result->command !=LIST ) {
            result->is_valid = false;
        }
    }

    buffer_read_adv(&client_Data->clientBuffer, readable_bytes);
    return result;
}

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

char * toLower(char * str) {
    for(int i = 0; str[i]; i++){
        str[i] = tolower(str[i]);
    }
    return str;
}

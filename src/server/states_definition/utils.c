#include "include/utils.h"

struct command_struct auth_user_commands[AUTH_COMMAND_AMOUNT] = {
    { .string = "user", .command = USER },
    { .string = "quit", .command = QUIT }
};

struct command_struct auth_pass_commands[AUTH_COMMAND_AMOUNT] = {
    { .string = "pass", .command = PASS },
    { .string = "quit", .command = QUIT }

};

struct command_struct transaction_commands[TRANSACTION_COMMAND_AMOUNT] = {
    { .string = "stat", .command = STAT },
    { .string = "list", .command = LIST },
    { .string = "retr", .command = RETR },
    { .string = "dele", .command = DELE },
    { .string = "noop", .command = NOOP },
    { .string = "rset", .command = RSET },
    { .string = "quit", .command = QUIT }

};

struct command_struct update_commands[UPDATE_COMMAND_AMOUNT] = {
    { .string = "quit", .command = QUIT }

};

struct possible_command_struct all_commands[COMMAND_AMOUNT] = {
    { .string = "user", .command = USER, .possible = TRUE, .has_params = TRUE },
    { .string = "pass", .command = PASS, .possible = TRUE, .has_params = TRUE  },
    { .string = "stat", .command = STAT, .possible = TRUE, .has_params = FALSE  },
    { .string = "list", .command = LIST, .possible = TRUE, .has_params = FALSE  }, // list can have argument
    { .string = "retr", .command = RETR, .possible = true, .has_params = TRUE  },
    { .string = "dele", .command = DELE, .possible = TRUE, .has_params = TRUE  },
    { .string = "noop", .command = NOOP, .possible = TRUE, .has_params = FALSE  },
    { .string = "rset", .command = RSET, .possible = TRUE, .has_params = FALSE  },
    { .string = "quit", .command = QUIT, .possible = TRUE, .has_params = FALSE  }

};

user_request * parse(struct selector_key * key, pop3_states state) {
    struct command_struct * commands_allowed;
    int commands_allowed_size = -1;

    switch (state)
    {
    case AUTHORIZATION_USER:
        commands_allowed = auth_user_commands;
        commands_allowed_size = AUTH_COMMAND_AMOUNT;
        break;
    case AUTHORIZATION_PASSWORD:
        commands_allowed = auth_pass_commands;
        commands_allowed_size = AUTH_COMMAND_AMOUNT;
        break;
    case TRANSACTION:
        commands_allowed = transaction_commands;
        commands_allowed_size = TRANSACTION_COMMAND_AMOUNT;
        break;
    case UPDATE:
        commands_allowed = update_commands;
        commands_allowed_size = UPDATE_COMMAND_AMOUNT;
        break;
    default:
        break;
    }
    
    client_data * clientData= ATTACHMENT(key);
    int command_index = 0;
    char command_entry[MAX_COMMAND_SIZE + 1];

    // Find the command requested
    uint8_t entry;
    do {
        entry = buffer_read(&clientData->clientBuffer);
        command_entry[command_index] = entry;
        command_index++;

    } while (command_index < MAX_COMMAND_SIZE && entry != ' ' && entry != '\n' && entry != '\0');
    
    command_entry[command_index] = '\0';

    boolean has_command_been_found = FALSE;

    // Find the valid command
    user_request * request = malloc(sizeof(user_request));
    request->is_valid = FALSE;
    request->arg = NULL;

    //Find if there is a command that matches the request
    for(int i = 0 ; i < COMMAND_AMOUNT  && !has_command_been_found; i++){
        if(strcmp(all_commands[i].string, toLower(command_entry)) == 0){
            has_command_been_found = TRUE;
            request->command = all_commands[i].command;
        }
    }

    if(!has_command_been_found) {
        return request;
    }

    //Was this a valid command in the current state?
    for (int i = 0; i < commands_allowed_size && !request->is_valid ; i++){
        if (commands_allowed[i].command == request->command) {
            request->is_valid = TRUE;
        }
    }
    if(!request->is_valid){
        return request;
    }
    
    //Get the arguments if needed
    if (all_commands[(int) request->command].has_params || request->command == LIST) {
        size_t param;
        buffer_read_ptr(&clientData->clientBuffer, &param);


        entry = buffer_read(&clientData->clientBuffer);
        if(entry == '\r' || entry == '\n'){ //No args where given
            entry = buffer_read(&clientData->clientBuffer);
            if(request->command != LIST){
                request->is_valid = FALSE;
            } else {
                request->arg = NULL;
                for(int i = 0; entry != '\r' && entry != '\n' && i < param; i++){
                    entry = buffer_read(&clientData->clientBuffer);
                }
                if(entry == '\r' || entry == '\n'){
                    entry = buffer_read(&clientData->clientBuffer);
                }
                return request;
            }
            return request;
        }

        if(entry != ' ') {
            request->is_valid = FALSE;
            return request;
        } else {// Skip the space
            entry = buffer_read(&clientData->clientBuffer);
        }
        
        request->arg = malloc((param+1) * sizeof(uint8_t));
        int i;
        for(i = 0; entry != '\r' && entry != '\n' && i < param; i++){
            if(entry == ' '){ // Skip the space
                request->is_valid = FALSE;
            }
            request->arg[i] = (char) entry;
            entry = buffer_read(&clientData->clientBuffer);
        }
        if(entry == '\r' || entry == '\n'){
            entry = buffer_read(&clientData->clientBuffer);
        }

        request->arg[i] = '\0';
    }

    return request;
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

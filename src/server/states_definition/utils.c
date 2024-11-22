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
    { .string = "user", .command = USER, .possible = true, .has_params = true },
    { .string = "pass", .command = PASS, .possible = true, .has_params = true  },
    { .string = "stat", .command = STAT, .possible = true, .has_params = false  },
    { .string = "list", .command = LIST, .possible = true, .has_params = false  }, // list can have argument
    { .string = "retr", .command = RETR, .possible = true, .has_params = true  },
    { .string = "dele", .command = DELE, .possible = true, .has_params = true  },
    { .string = "noop", .command = NOOP, .possible = true, .has_params = false  },
    { .string = "rset", .command = RSET, .possible = true, .has_params = false  },
    { .string = "quit", .command = QUIT, .possible = true, .has_params = false  }

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

    char * command_entry = malloc(MAX_COMMAND_SIZE + 1);
    uint8_t entry;
    int command_index = 0;
    do {
        entry = buffer_read(&clientData->clientBuffer);
        command_entry[command_index] = entry;
        command_index++;

    } while (command_index < MAX_COMMAND_SIZE && entry != ' ' && entry != '\n' && entry != '\0');
    
    command_entry[command_index] = '\0';

    boolean has_command_been_found = FALSE;

    // Find the valid command
    user_request * request = malloc(sizeof(user_request));

    //Find if there is a command that matches the request
    for(int i = 0 ; i < COMMAND_AMOUNT ; i++){
        if(strcmp(all_commands[i].string, command_entry) == 0) {
            has_command_been_found = TRUE;
            request->command = all_commands[i].command;
        }
    }

    if(!has_command_been_found) {
        free(command_entry);
        return NULL;
    }

    //Was this a valid command in the current state?
    for (int i = 0; i < commands_allowed_size && !request->is_allowed ; i++){
        if (commands_allowed[i].command == request->command) {
            request->is_allowed = true;
        }
    }
    if(!request->is_allowed){
        free(command_entry);
        return request;
    }
    

    //Get the arguments if needed
    if (all_commands[(int) request->command].has_params || request->command == LIST) {
        size_t param;
        buffer_read_ptr(&clientData->clientBuffer, &param);

        entry = buffer_read(&clientData->clientBuffer);
        request->arg = malloc((param+1) * sizeof(uint8_t));
        int i;
        for(i = 0; entry != '\r' && entry != '\n' && i < param; i++){
            request->arg[i] = (char) entry;
            entry = buffer_read(&clientData->clientBuffer);
        }
        request->arg[i] = '\0';
    }

    free(command_entry);

    return request;
}

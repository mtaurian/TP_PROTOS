#include "./include/transaction.h"

void transaction_on_arrival(unsigned state, struct selector_key *key){
  	printf("[POP3] Entered in TRANSACTION state\n");
}

void transaction_on_departure(unsigned state, struct selector_key *key){
  	printf("[POP3] Exited TRANSACTION state\n");
}

unsigned int transaction_on_ready_to_read(struct selector_key *key){
  	user_request * entry = parse(key);
    char * message = malloc(MAX_RESPONSE_SIZE);
  	int ret = TRANSACTION;
	if(entry->command == INVALID){
        write_error_message(key, UNKNOWN_COMMAND);
		return ret;
	}


  	switch (entry->command) {
    	case STAT:
      		handle_stat(key);
      		break;
    	case LIST:
      		handle_list(key, entry->arg);
      		break;
    	case RETR:
      		if(!handle_retr(key, entry->arg)){ // error
				write_std_response(0,  "no such message\r\n",key);
      		}
      		break;
    	case DELE:
      		if(!handle_dele(key, entry->arg)){
                snprintf(message, MAX_RESPONSE_SIZE, "message %d already deleted\r\n", atoi(entry->arg)); // TODO: validacion con negativos
				write_std_response(0, message, key);
      		}
      		break;
    	case NOOP:
      		write_std_response(OK, NULL, key);
      		break;
    	case RSET:
      		int rset_amount = handle_rset(key);
            snprintf(message, MAX_RESPONSE_SIZE, "maildrop has %d messages\r\n", rset_amount);
            write_std_response(1, message, key);
      		break;
    	case QUIT:
            ret = UPDATE;
      		break;
    	default:
      		write_error_message(key, UNKNOWN_COMMAND);
      		break;
  	}

    free(message);
  	free(entry);

  	return ret;
}

unsigned int transaction_on_ready_to_write(struct selector_key *key){
  	return ATTACHMENT(key)->stm.current->state;
}

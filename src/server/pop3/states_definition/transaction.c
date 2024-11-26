#include "./include/transaction.h"

void transaction_on_arrival(unsigned state, struct selector_key *key){
  	printf("[POP3] Entered in TRANSACTION state\n");
}

void transaction_on_departure(unsigned state, struct selector_key *key){
  	printf("[POP3] Exited TRANSACTION state\n");
}

unsigned int transaction_on_ready_to_read(struct selector_key *key){
  	user_request entry = parse(key);
  	int ret = TRANSACTION;
    
	if(entry.command == INVALID){
        write_error_message(key, UNKNOWN_COMMAND);
		return ret;
	}


  	switch (entry.command) {
    	case STAT:
      		handle_stat(key);
      		break;
    	case LIST:
      		handle_list(key, entry.arg);
      		break;
    	case RETR:
      		handle_retr(key, entry.arg);
      		break;
    	case DELE:
      		handle_dele(key, entry.arg);
      		break;
    	case NOOP:
      		write_std_response(OK, NULL, key);
      		break;
    	case RSET:
      		handle_rset(key);
      		break;
    	case QUIT:
            ret = UPDATE;
      		break;
    	default:
      		write_error_message(key, UNKNOWN_COMMAND);
      		break;
  	}

  	return ret;
}

unsigned int transaction_on_ready_to_write(struct selector_key *key){
  	return ATTACHMENT(key)->stm.current->state;
}

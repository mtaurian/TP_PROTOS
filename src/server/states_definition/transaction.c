#include "./include/transaction.h"

void transaction_on_arrival(unsigned state, struct selector_key *key){
  	printf("Entered in TRANSACTION state\n");
}

void transaction_on_departure(unsigned state, struct selector_key *key){
  	printf("Exited TRANSACTION state\n");
}

unsigned int transaction_on_ready_to_read(struct selector_key *key){
  	user_request * entry = parse(key, TRANSACTION);
    char * message = malloc(MAX_RESPONSE_SIZE);
  	int ret = TRANSACTION;

  	const char * statCmd = "STAT";
  	const char * listCmd = "LIST";
  	const char * retrCmd = "RETR";
  	const char * deleCmd = "DELE";
  	const char * noopCmd = "NOOP";
  	const char * rsetCmd = "RSET";
  	const char * quitCmd = "QUIT";


  	switch (entry->command) {
    	case STAT:
      		handle_stat(key);
      		break;
    	case LIST:
      		if(!handle_list(key, entry->arg)){
                snprintf(message, MAX_RESPONSE_SIZE, "no such message, only %d %s in maildrop\r\n", ATTACHMENT(key)->user->mailbox->mail_count, ATTACHMENT(key)->user->mailbox->mail_count > 1 ? "messages" : "message");
				write_std_response(0, message, key);
      		}
      		break;
    	case RETR:
      		if(handle_retr(key, entry->arg)){

      		} else { // error
				write_std_response(0,  "no such message\r\n",key);
      		}
      		break;
    	case DELE:
      		if(handle_dele(key, entry->arg)){

      		} else { // error

      		}
      		break;
    	case NOOP:
      		write_std_response(1, NULL, key);
      		break;
    	case RSET:
      		if(handle_rset(key)){

      		} else { // error

      		}
      		break;
    	case QUIT:
     		handle_quit(key);
     		snprintf(message, MAX_RESPONSE_SIZE, "Goodbye\n");
      		write_std_response(1,message, key);
      		break;
    	default:
      		write_std_response(0, NULL, key);
      		break;
  	}

  	if(entry->arg){
    	free(entry->arg);
  	}

    free(message);
  	free(entry);

  	return ret;
}

unsigned int transaction_on_ready_to_write(struct selector_key *key){
  	return ATTACHMENT(key)->stm.current->state;
}

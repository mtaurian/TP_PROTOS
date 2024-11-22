#include "./include/transaction.h"

void transaction_on_arrival(unsigned state, struct selector_key *key){
  printf("Entered in TRANSACTION state\n");
}

void transaction_on_departure(unsigned state, struct selector_key *key){
  printf("Exited TRANSACTION state\n");
}

unsigned int transaction_on_ready_to_read(struct selector_key *key){
  user_request * entry = parse(key, TRANSACTION);
  char * message = NULL;
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
      if(handle_stat(key)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case LIST:
      if(handle_list(key, entry->arg)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case RETR:
      if(handle_retr(key, entry->arg)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case DELE:
      if(handle_dele(key, entry->arg)){

      } else { // error

      }
      ret = TRANSACTION;
      break;

    case NOOP:
      if(handle_noop(key)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case RSET:
      if(handle_rset(key)){

      } else { // error

      }
      ret = TRANSACTION;
      break;

    case QUIT:
      handle_quit(key);
      message =  "Goodbye\n";
      write_std_response(1,message, key);
      break;
    default:
      write_std_response(0, NULL, key);
      break;
  }

  free(entry->arg);
  free(entry);

  return ret;
}

unsigned int transaction_on_ready_to_write(struct selector_key *key){
  return 0;
}

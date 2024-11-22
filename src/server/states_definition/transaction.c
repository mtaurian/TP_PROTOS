#include "./include/transaction.h"


static transaction_request * parse(struct selector_key * key){
  client_data * clientData= ATTACHMENT(key);

  uint8_t entry = buffer_read(&clientData->clientBuffer);

  //TODO: Change all this command compares to parse_utils :)
  char match = 1;
  char maybePass = 1;
  char maybeQuit = 1;
  const char * statCmd = "STAT";
  const char * listCmd = "LIST";
  const char * retrCmd = "RETR";
  const char * deleCmd = "DELE";
  const char * noopCmd = "NOOP";
  const char * rsetCmd = "RSET";
  const char * quitCmd = "QUIT";

//  while(match && entry != ' ' && entry) {
//    if (maybePass && (entry == *userCmd || entry == *userCmd + 32)) {
//      userCmd++;
//      maybeQuit = 0;
//    } else if (maybeQuit && (entry == *quitCmd || entry == *quitCmd + 32)) {
//      quitCmd++;
//      maybePass = 0;
//    } else {
//      match = 0;
//    }
//    entry = buffer_read(&clientData->clientBuffer);
//  }

  transaction_request *request = malloc(sizeof(transaction_request));

//  if(match){
//    request->command = maybeQuit ? QUIT_PASS : PASS;
//  }

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

void transaction_on_arrival(unsigned state, struct selector_key *key){
  printf("Entered in TRANSACTION state\n");
}

void transaction_on_departure(unsigned state, struct selector_key *key){
  printf("Exited TRANSACTION state\n");
}

unsigned int transaction_on_ready_to_read(struct selector_key *key){
  transaction_request * entry = parse(key);
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
      if(handle_list(key, entry->payload)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case RETR:
      if(handle_retr(key, entry->payload)){

      } else { // error

      }
      ret = TRANSACTION;
      break;
    case DELE:
      if(handle_dele(key, entry->payload)){

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

    case QUIT_TRANSACTION:
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

  return ret;
}

unsigned int transaction_on_ready_to_write(struct selector_key *key){
  return 0;
}

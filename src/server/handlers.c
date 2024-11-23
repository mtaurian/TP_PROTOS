#include "include/handlers.h"


int handle_user(struct selector_key *_key, char * username){
    client_data * clientData = ATTACHMENT(_key);
    unsigned long userNameLength = strlen(username);

    clientData->username = malloc((userNameLength+1) * sizeof(char));
    strcpy(clientData->username, username);

    printf("Username set: %s\n", username);
    return 1;
}

void handle_quit(struct selector_key *key){
    close_client(key);
}

int handle_pass(struct selector_key *_key, char * password){
    client_data * clientData = ATTACHMENT(_key);
    unsigned long length = strlen(password);

    clientData->password = malloc((length+1) * sizeof(char));
    strcpy(clientData->password, password);

    clientData->user = validate_user(clientData->username, clientData->password);
    if(clientData->user && log_user(clientData->user)){
        return 1;
    }
    return 0;
}

int handle_stat(struct selector_key *_key){
  printf("STAT\n");
  return 1;
}

int handle_list(struct selector_key *_key, char * mail_number){
  printf("LIST\n");
  return 1;
}

int handle_retr(struct selector_key *_key, char * mail_number){
  printf("RETR\n");
  return 1;
}

int handle_dele(struct selector_key *_key, char * mail_number){
  printf("DELE\n");
  return 1;
}

int handle_rset(struct selector_key *_key){
  printf("RSET\n");
  return 1;
}


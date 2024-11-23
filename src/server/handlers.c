#include "include/handlers.h"


int handle_user(struct selector_key *key, char * username){
    client_data * clientData = ATTACHMENT(key);
    unsigned long userNameLength = strlen(username);

    clientData->username = malloc((userNameLength+1) * sizeof(char));
    strcpy(clientData->username, username);

    printf("Username set: %s\n", username);
    return 1;
}

void handle_quit(struct selector_key *key){
    close_client(key);
}

int handle_pass(struct selector_key *key, char * password){
    client_data * clientData = ATTACHMENT(key);
    unsigned long length = strlen(password);

    clientData->password = malloc((length+1) * sizeof(char));
    strcpy(clientData->password, password);

    clientData->user = validate_user(clientData->username, clientData->password);
    if(clientData->user && log_user(clientData->user)){
        return 1;
    }
    return 0;
}

int handle_stat(struct selector_key *key){
  	client_data * clientData = ATTACHMENT(key);
    int total_size = 0;
	int visible_mails = 0; // mails that are not marked as deleted
    t_mailbox * mailbox = clientData->user->mailbox;


    for (int i = 0; i < mailbox->mail_count; i++) {
        if (!mailbox->mails[i].deleted) {
            total_size += mailbox->mails[i].size;
            visible_mails++;
        }
    }


    char response[128];
    snprintf(response, sizeof(response), "%d %d\r\n", visible_mails, total_size);
    write_std_response(1,response, key);
  	return 1;
}

int handle_list(struct selector_key *key, char * mail_number){
  	printf("LIST\n");
  	return 1;
}

int handle_retr(struct selector_key *key, char * mail_number){
  	printf("RETR\n");
  	return 1;
}

int handle_dele(struct selector_key *key, char * mail_number){
  	printf("DELE\n");
  	return 1;
}

int handle_rset(struct selector_key *key){
  	printf("RSET\n");
  	return 1;
}


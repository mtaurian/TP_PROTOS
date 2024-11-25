#ifndef TP_PROTOS_HANDLERS_H
#define TP_PROTOS_HANDLERS_H

#include "../../../shared/include/selector.h"
#include "../../../shared/include/utils.h"

#define MAX_RESPONSE_SIZE 1024

void handle_quit(struct selector_key *key);

int handle_user(struct selector_key *key, char * username);

int handle_pass(struct selector_key *key, char * password);

int handle_stat(struct selector_key *key);
int handle_list(struct selector_key *key, char * mail_number);
int handle_retr(struct selector_key *key, char * mail_number);
void handle_dele(struct selector_key *key, char * mail_number);
int handle_rset(struct selector_key *key);

void handle_update_quit(struct selector_key *key);
#endif //TP_PROTOS_HANDLERS_H

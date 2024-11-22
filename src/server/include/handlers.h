#ifndef TP_PROTOS_HANDLERS_H
#define TP_PROTOS_HANDLERS_H

#include "../../shared/include/selector.h"
#include "../states_definition/include/utils.h"

void handle_quit(struct selector_key *key);

int handle_user(struct selector_key *_key, char * username);

int handle_pass(struct selector_key *_key, char * password);

int handle_stat(struct selector_key *_key);
int handle_list(struct selector_key *_key, char * mail_number);
int handle_retr(struct selector_key *_key, char * mail_number);
int handle_dele(struct selector_key *_key, char * mail_number);
int handle_noop(struct selector_key *_key);
int handle_rset(struct selector_key *_key);

#endif //TP_PROTOS_HANDLERS_H

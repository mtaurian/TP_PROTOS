#ifndef TP_PROTOS_HANDLERS_H
#define TP_PROTOS_HANDLERS_H

#include "../../shared/include/selector.h"

void handle_quit(struct selector_key *key);
int handle_user(struct selector_key *_key, char * username);
int handle_pass(struct selector_key *_key, char * password);
#endif //TP_PROTOS_HANDLERS_H

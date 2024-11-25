#ifndef TP_PROTOS_MGMT_HANDLERS_H
#define TP_PROTOS_MGMT_HANDLERS_H

#include "../../../shared/include/utils.h"
#include "../../pop3/include/pop3.h"
#include "mgmt.h"

boolean handle_login(struct selector_key *key, char *arg);
boolean handle_users(struct selector_key *key);
boolean handle_add_user(struct selector_key * key, char * arg);
boolean handle_delete_user(struct selector_key * key, char * arg);

#endif //TP_PROTOS_MGMT_HANDLERS_H
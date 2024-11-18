#ifndef TP_PROTOS_UTILS_H
#define TP_PROTOS_UTILS_H

#include "../../include/pop3.h"

void print_response(struct selector_key * key);
void fd_to_client_buffer(client_data * clientData, struct selector_key * key);
void write_std_response(char isOk, char * msg, struct selector_key * key);
#endif //TP_PROTOS_UTILS_H

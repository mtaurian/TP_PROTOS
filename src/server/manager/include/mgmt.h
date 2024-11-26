#ifndef TP_PROTOS_MGMT_H
#define TP_PROTOS_MGMT_H

#include <stddef.h>
#include <unistd.h>
#include <memory.h>
#include <sys/socket.h>
#include <stdint.h>
#include "../../../shared/include/selector.h"
#include "../../../shared/include/buffer.h"
#include "../../../shared/include/stm.h"

#define BUFFER_MAX_SIZE 2048
#define MAX_MGMT_USERS 10

typedef struct super_user_data {
    char *name;
    char *pass;
    unsigned int logged;
} super_user_data;

struct mgmt_server {
    super_user_data users_list[MAX_MGMT_USERS];
    unsigned int admin_amount;
};


enum mgmt_states {
    INIT = 0,
    NON_AUTHENTICATED,
    AUTHENTICATED
};

void initialize_mgmt_server();
void free_mgmt_server();


void mgmt_passive_accept(struct selector_key *_key);
void close_mgmt_client(struct selector_key * _key);
super_user_data * validate_admin(const char *name, const char *pass);
void mgmt_user(const char *s);


#endif //TP_PROTOS_MGMT_H

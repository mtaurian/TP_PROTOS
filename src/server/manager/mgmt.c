#include <stdlib.h>
#include "include/mgmt.h"
#include "../../shared/include/utils.h"
#include "manager_states_definition/include/init_mgmt.h"
#include "manager_states_definition/include/non_authenticated_mgmt.h"
#include "manager_states_definition/include/authenticated_mgmt.h"

static struct mgmt_server * mgmt_server;

static const struct state_definition states [] = {
        {
            .state            = INIT,
            .on_arrival       = init_on_arrival,
            .on_departure     = init_on_departure,
            .on_read_ready    = NULL,
            .on_write_ready   = init_on_ready_to_write,
        },
        {
            .state            = NON_AUTHENTICATED,
            .on_arrival       = non_authenticated_on_arrival,
            .on_departure     = non_authenticated_on_departure,
            .on_read_ready    = non_authenticated_on_read_ready ,
            .on_write_ready   = non_authenticated_on_write_ready,
        },
        {
            .state            = AUTHENTICATED,
            .on_arrival       = authenticated_on_arrival,
            .on_departure     = authenticated_on_departure,
            .on_read_ready    = authenticated_on_read_ready,
            .on_write_ready   = authenticated_on_write_ready,
        }
    };

static const struct fd_handler client_handler = {
        .handle_read   =  read_handler,
        .handle_write  =  write_handler,
        .handle_close  = close_mgmt_client,
        .handle_block = NULL,
};


void initialize_mgmt_server() {
    mgmt_server = malloc(sizeof(struct mgmt_server));
    mgmt_server->admin_amount = 0;
}

void free_mgmt_server() {
    for (int i = 0; i < mgmt_server->admin_amount; i++) {
        const super_user_data *user = &mgmt_server->users_list[i];
        free(user->name);
        free(user->pass);
    }
    free(mgmt_server);
}

void mgmt_passive_accept(struct selector_key *_key) {
    const char * err_msg = NULL;
    struct sockaddr_storage mgmt_addr;
    socklen_t client_addr_len = sizeof(mgmt_addr);
    memset(&mgmt_addr, 0, sizeof(mgmt_addr));
    int client_fd = accept(_key->fd, (struct sockaddr *) &mgmt_addr, &client_addr_len);
    selector_status ss = SELECTOR_SUCCESS;

    if (client_fd < 0) {
        err_msg = "Error in accept - mgmt\n";
        goto finally;
    }

    if (selector_fd_set_nio(client_fd) == -1) {
        err_msg = "Unable to set client socket flags - mgmt \n";
        goto finally;
    }

    client_data *clientData = calloc(1, sizeof(client_data));

    if (clientData == NULL) {
        return;
    }

    clientData->closed = false;
    clientData->clientFd = client_fd;
    clientData->clientAddress = mgmt_addr;
    clientData->username = NULL;
    clientData->password = NULL;
    clientData->stm.states = states;
    buffer_init(&clientData->clientBuffer, BUFFER_MAX_SIZE, clientData->inClientBuffer);
    buffer_init(&clientData->responseBuffer, BUFFER_MAX_SIZE, clientData->inResponseBuffer);

    clientData->stm.initial = INIT;
    clientData->stm.max_state = AUTHENTICATED;
    stm_init(&clientData->stm);
    ss = selector_register(_key->s, client_fd, &client_handler, OP_WRITE, clientData);

    if (ss != SELECTOR_SUCCESS) {
        err_msg = "[MGMT] Unable to register client socket handler";
    }

    finally:
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", err_msg,
                ss == SELECTOR_IO
                ? strerror(errno)
                : selector_error(ss));
    } else if(err_msg) {
        perror(err_msg);
        free(clientData);
        close(client_fd);
    } else {
        printf("[MGMT] Client connected\n");
    }

}

void close_mgmt_client(struct selector_key * _key) {
    client_data* data = ATTACHMENT(_key);
    if (data->closed)
        return;

    data->closed = true;

    int clientFd = data->clientFd;

    if (clientFd != -1) {
        selector_unregister_fd(_key->s, clientFd);
        close(clientFd);
    }

    free(data->password);
    free(data->username);
    free(data);
}


void mgmt_user(const char *s) {
    super_user_data *user = &mgmt_server->users_list[mgmt_server->admin_amount];
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        size_t name_length = strlen(s);
        size_t pass_length = strlen(p);
        char * name = malloc(sizeof(char)*(name_length+1));
        char * pass = malloc(sizeof(char)*(pass_length+1));
        strcpy(name, s);
        strcpy(pass, p);
        user->pass = pass;
        user->name = name;
        user->logged = 0;
        mgmt_server->admin_amount++;

        printf("[MGMT] User %s:%s added\n", name, pass);
    }
}

super_user_data * validate_admin(const char *name, const char *pass){
    for(int i = 0; i < mgmt_server->admin_amount; i++) {
        super_user_data *user = &mgmt_server->users_list[i];
        if(user->name == NULL || user->pass == NULL) {
            return NULL;
        }
        if(strcmp(user->name, name) == 0 && strcmp(user->pass, pass) == 0) {
            return &mgmt_server->users_list[i];
        }
    }
    return NULL;
}
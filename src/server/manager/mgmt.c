#include <stdlib.h>
#include "include/mgmt.h"
#include "../states_definition/include/utils.h"
#include "manager_states_definition/include/non_authenticated_mgmt.h"

static const struct state_definition states [] = {
        {
            .state            = NON_AUTHENTICATED,
            .on_arrival       = non_authenticated_on_arrival,
            .on_departure     = NULL,
            .on_read_ready    = NULL,
            .on_write_ready   = NULL,
        },
        {
            .state            = AUTHENTICATED,
            .on_arrival       = NULL,
            .on_departure     = NULL,
            .on_read_ready    = NULL,
            .on_write_ready   = NULL,
        }
    };

static const struct fd_handler client_handler = {
        .handle_read   =  read_handler,
        .handle_write  =  write_handler,
        .handle_close  = close_mgmt_client,
        .handle_block = NULL,
};


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

    clientData->stm.initial = NON_AUTHENTICATED;
    clientData->stm.max_state = AUTHENTICATED;
    stm_init(&clientData->stm);
    ss = selector_register(_key->s, client_fd, &client_handler, OP_READ, clientData);

    if (ss != SELECTOR_SUCCESS) {
        err_msg = "Unable to register client socket handler";
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
        printf("Client connected\n");
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
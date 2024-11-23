#include "include/handlers.h"
#include <fcntl.h>
#include <unistd.h>


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
	int visible_mails = 0; // mails that are not marked as deleted
    t_mailbox * mailbox = clientData->user->mailbox;

    char response[128];
    snprintf(response, sizeof(response), "%d %zd\r\n", clientData->user->mailbox->mail_count - clientData->user->mailbox->deleted_count , clientData->user->mailbox->mails_size);
    write_std_response(1,response, key);
  	return 1;
}

int handle_list(struct selector_key *key, char * mail_number){
  	int mail_id = atoi(mail_number);
    client_data * clientData = ATTACHMENT(key);
    t_mailbox * mailbox = clientData->user->mailbox;

    char *response = malloc(MAX_RESPONSE_SIZE);

    if (mail_id > 0) {
        if (mail_id <= mailbox->mail_count && !mailbox->mails[mail_id - 1].deleted) {
            snprintf(response, sizeof(response), "%d %zu\r\n", mail_id, mailbox->mails[mail_id - 1].size);
            write_std_response(1,response, key);
        } else {
            free(response);
          	return 0;
        }
        free(response);
        return 1;
    }

    snprintf(response, MAX_RESPONSE_SIZE, "%d messages (%zu octets)\r\n", mailbox->mail_count, mailbox->mails_size);

	for (int i = 0; i < mailbox->mail_count; i++) {
    	if (!mailbox->mails[i].deleted) {
        	char mail_info[50];
        	snprintf(mail_info, sizeof(mail_info), "%d %zu\r\n", mailbox->mails[i].id, mailbox->mails[i].size);
        	strcat(response, mail_info);
    	}
	}
    strcat(response, ".\r\n");

	write_std_response(1, response, key);

    free(response);

	return 1;
}

int handle_retr(struct selector_key *key, char *mail_number) {
	client_data *clientData = ATTACHMENT(key);
	t_mailbox *mailbox = clientData->user->mailbox;

	int mail_id = atoi(mail_number);

	if (mail_id <= 0 || mail_id > mailbox->mail_count || mailbox->mails[mail_id - 1].deleted) {
		write_std_response(0, "-ERR Invalid message number\r\n", key);
		return 0;
	}

	mail *mail = &mailbox->mails[mail_id - 1];

	if (mail->fd < 0) {
		mail->fd = open(mail->filename, O_RDONLY | O_NONBLOCK);
		if (mail->fd < 0) {
			perror("Error opening mail file");
			write_std_response(0, "-ERR Could not open mail file\r\n", key);
			return 0;
		}
	}

	char * buffer = malloc(BUFFER_SIZE);
    char * response = malloc(BUFFER_SIZE);
	size_t response_len = 0;

	strcpy(response, "message follows\r\n");
	response_len = strlen(response);

	ssize_t bytes_read;
	while ((bytes_read = read(mail->fd, buffer, BUFFER_SIZE)) > 0) {
		buffer[bytes_read] = '\0';
		if (response_len + bytes_read + 1 > BUFFER_SIZE) {
			response = realloc(response, response_len + bytes_read + 1);
		}
		strncat(response, buffer, bytes_read);
		response_len += bytes_read;
	}

	if (bytes_read < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			free(buffer);
			free(response);
			return 0;
		} else {
			perror("Error reading mail file");
			close(mail->fd);
			mail->fd = -1;
			write_std_response(0, "-ERR Could not read mail file\r\n", key);
			free(buffer);
			free(response);
			return 0;
		}
	}

	strcat(response, "\n.\r\n");
	write_std_response(1, response, key);

	close(mail->fd);
	mail->fd = -1;

	free(response);
	free(buffer);

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


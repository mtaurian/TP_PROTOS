#include "include/handlers.h"
#include <fcntl.h>
#include <unistd.h>


int handle_user(struct selector_key *key, char * username){
    client_data * clientData = ATTACHMENT(key);
    unsigned long userNameLength = strlen(username);

    clientData->username = malloc((userNameLength+1) * sizeof(char));
    strcpy(clientData->username, username);

    printf("[POP3] Username set: %s\n", username);
    return 1;
}

void handle_quit(struct selector_key *key){
    close_client(key);
	write_std_response(1,"Goodbye", key);
}

int handle_pass(struct selector_key *key, char * password){
    client_data * clientData = ATTACHMENT(key);
    unsigned long length = strlen(password);

    clientData->user = validate_user(clientData->username, password);
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
    snprintf(response, sizeof(response), "%d %zd\r\n", clientData->user->mailbox->mail_count , clientData->user->mailbox->mails_size);
    write_std_response(1,response, key);
  	return 1;
}

int handle_list(struct selector_key *key, char * mail_number){
  	int mail_id;
    client_data * clientData = ATTACHMENT(key);
    t_mailbox * mailbox = clientData->user->mailbox;

    if(mail_number == NULL) {
        mail_id = -1;
    } else {
        mail_id = atoi(mail_number);   // TODO: list -1 returns same error as the rest
    }

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

	for (int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++) {
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

	int mail_id = atoi(mail_number); // TODO: atoi breaks when float

	if (mail_id <= 0 || mail_id > mailbox->mail_count || mailbox->mails[mail_id - 1].deleted) {
		return 0; // TODO eror management
	}

	mail *mail = &mailbox->mails[mail_id - 1];

	if (mail->fd < 0) {
		mail->fd = open(mail->filename, O_RDONLY | O_NONBLOCK);
		if (mail->fd < 0) {
			perror("Error opening mail file");
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
             //
		} else {
			perror("Error reading mail file"); // TODO error management
			write_std_response(0, "-ERR Could not read mail file\r\n", key);
		}
        free(response);
		free(buffer);
        mail->fd = -1;
    	close(mail->fd);
		return 0;

	}

	strcat(response, "\n.\r\n");
	write_std_response(1, response, key);

	close(mail->fd);
	mail->fd = -1;

	free(response);
	free(buffer);

	return 1;
}



void handle_dele(struct selector_key *key, char * mail_number){
    client_data * clientData = ATTACHMENT(key);
	t_mailbox * mailbox = clientData->user->mailbox;

    long mail_id = 0;

    char * endptr;
    if(mail_number == NULL) {
        mail_id = -1;
    } else {
        mail_id = strtol(mail_number, &endptr, 10);
    }
    if (*endptr != '\0') {
        write_error_message_with_arg(key, NOICE_AFTER_MESSAGE, endptr);
        return;
    }

    if(mail_id == -1){
        write_error_message_with_arg(key, INVALID_MESSAGE_NUMBER, endptr);
        return;
    }

    if(mail_id > mailbox->mail_count){
        write_error_message_with_arg(key, NO_MESSAGE, endptr);
        return;
    }

    if(mailbox->mails[mail_id - 1].deleted){
        write_error_message_with_arg(key, MESSAGE_ALREADY_DELETED, endptr);
		return;
	}

    mailbox->mails[mail_id - 1].deleted = TRUE;
    mailbox->mails_size -= mailbox->mails[mail_id - 1].size;
    mailbox->mail_count--;
    mailbox->deleted_count++;
    
	write_ok_message(key, MARKED_TO_BE_DELETED);
  	return;
}

void handle_rset(struct selector_key *key){
  	client_data * clientData = ATTACHMENT(key);
	t_mailbox * mailbox = clientData->user->mailbox;

    int rset_amount = 0;
    for(int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++){
		if(mailbox->mails[i].deleted){
      		mailbox->mails[i].deleted = FALSE;
            mailbox->mails_size += clientData->user->mailbox->mails[i].size;
            mailbox->mail_count++;
            mailbox->deleted_count--;
        }
	}
    write_ok_message(key, JUST_OK);
    return;
}

void handle_update_quit(struct selector_key *key){
	client_data * clientData = ATTACHMENT(key);
    t_mailbox * mailbox = clientData->user->mailbox;
	for (int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++) {
		if (mailbox->mails[i].deleted) {
			remove(mailbox->mails[i].filename);
		}
	}
    handle_quit(key);
}

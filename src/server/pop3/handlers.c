#include "include/handlers.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int handle_user(struct selector_key *key, char * username){
    client_data * clientData = ATTACHMENT(key);
    unsigned long userNameLength = strlen(username);

    clientData->username = malloc((userNameLength+1) * sizeof(char));
    strcpy(clientData->username, username);

    printf("[POP3] Username set: %s\n", username);
    return OK;
}

void handle_quit(struct selector_key *key){
    client_data * clientData = ATTACHMENT(key);
    clientData->readyToLogout = TRUE;
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
    write_std_response(OK,response, key);
  	return OK;
}

void handle_list(struct selector_key *key, char * mail_number){
    client_data * clientData = ATTACHMENT(key);
    t_mailbox * mailbox = clientData->user->mailbox;

  	long mail_id;
	char response[MAX_RESPONSE_SIZE];
	char *endptr;

    if(*mail_number != '\0' ){
        mail_id = strtol(mail_number, &endptr, 10);
		if (*endptr != '\0') {
			write_error_message_with_arg(key, NOICE_AFTER_MESSAGE, endptr);
			endptr = NULL;
			return;
		}
	    if (mail_id > 0) {
    		printf("[POP3] List mail number:%ld\n", mail_id);
	        if (mail_id <= mailbox->mail_count){
	        	if(!mailbox->mails[mail_id - 1].deleted) {
	        		snprintf(response, sizeof(response), "%ld %zu", mail_id, mailbox->mails[mail_id - 1].size);
	        		write_std_response(OK,response, key);
	        		return;
	        	}	else {
	        		write_error_message(key,MESSAGE_ALREADY_DELETED);
	        		return;
	        	}
	        } else {
				write_error_message_with_arg(key,NO_MESSAGE,mail_number);
        		return;
	        }
	    } else {
			write_error_message_with_arg(key,INVALID_MESSAGE_NUMBER,mail_number);
	    	return;
	    }
    }



    snprintf(response, MAX_RESPONSE_SIZE, "%d messages (%zu octets)\r\n", mailbox->mail_count, mailbox->mails_size);

	for (int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++) {
    	if (!mailbox->mails[i].deleted) {
        	char mail_info[50];
        	snprintf(mail_info, sizeof(mail_info), "%d %zu\r\n", mailbox->mails[i].id, mailbox->mails[i].size);
        	strcat(response, mail_info);
    	}
	}
    strcat(response, ".");

	write_std_response(OK, response, key);

	return;
}

void handle_retr(struct selector_key *key, char *mail_number) {
	client_data *clientData = ATTACHMENT(key);

	t_mailbox *mailbox = clientData->user->mailbox;
	long mail_id;
	char *endptr;
	if (*mail_number != '\0') {
		mail_id = strtol(mail_number, &endptr, 10);
		if (*endptr != '\0') {
			write_error_message_with_arg(key, NOICE_AFTER_MESSAGE, endptr);
			endptr = NULL;
			return;
		}
		if (mail_id > 0) {
			if (mail_id <= mailbox->mail_count) {
				if (!mailbox->mails[mail_id - 1].deleted) {
					mail *mail = &mailbox->mails[mail_id - 1];
					if (mail->fd < 0) {
						mail->fd = open(mail->filename, O_RDONLY | O_NONBLOCK);
						if (mail->fd < 0) {
							printf("[POP3] Error opening mail file");
							write_error_message(key, COULD_NOT_READ_MAIL_FILE);
							return;
						}
					}

					char *buffer = malloc(BUFFER_SIZE);
					char *response = malloc(BUFFER_SIZE);
					size_t response_len = 0;

					strcpy(response, "message follows\r\n");
					response_len = strlen(response);

					ssize_t bytes_read;
                    int transformation_fd = transform_mail_piping(mail->fd);
					while ((bytes_read = read(transformation_fd, buffer, BUFFER_SIZE)) > 0) {
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
							printf("[POP3] Error reading mail file");
							write_error_message(key, COULD_NOT_READ_MAIL_FILE);
						}
						free(response);
						free(buffer);
						mail->fd = -1;
						close(mail->fd);
						return;
					}

					strcat(response, "\r\n.");
					write_std_response(OK, response, key);

					close(mail->fd);
					mail->fd = -1;

					free(response);
					free(buffer);

					return;
				} else {
					write_error_message(key, MESSAGE_ALREADY_DELETED);
					return;
				}
			} else {
				write_error_message_with_arg(key, NO_MESSAGE, mail_number);
				return ;
			}
		}
	}
	write_error_message_with_arg(key, INVALID_MESSAGE_NUMBER, mail_number);
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

    if(mail_id > mailbox->mail_count + mailbox->deleted_count){
        write_error_message_with_arg(key, NO_MESSAGE, endptr);
        return;
    }

    if(mailbox->mails[mail_id - 1].deleted){
        write_error_message_with_arg(key, MESSAGE_ALREADY_DELETED, endptr);
		return;
	}

	// Move message to cur folder
    char *oldPath = mailbox->mails[mail_id - 1].filename;
    char * filename = strrchr(oldPath, '/');
    char * newPath = malloc(strlen(oldPath) + 2);
    char  * maildir = get_maildir();
    snprintf(newPath, strlen(oldPath) + 2, "%s/%s/cur/%s", maildir, clientData->username, filename);

    //si no existe cur la creamos
    char * user_maildir = malloc(strlen(maildir)+strlen(clientData->username)+2+4);
    sprintf(user_maildir, "%s/%s/cur", maildir, clientData->username);
    mkdir(user_maildir, 0777); //if it already exits does not matter
    free(user_maildir);

    mailbox->mails[mail_id - 1].filename = newPath;
    if (rename(oldPath, newPath) != 0) {
        write_error_message(key, INTERNAL_ERROR);
        free(oldPath);
        return;
    }
    mailbox->mails[mail_id - 1].deleted = TRUE;
    mailbox->mails_size -= mailbox->mails[mail_id - 1].size;
    mailbox->mail_count--;
    mailbox->deleted_count++;
    write_ok_message(key, MARKED_TO_BE_DELETED);
    free(oldPath);
}

void handle_rset(struct selector_key *key){
  	client_data * clientData = ATTACHMENT(key);
	t_mailbox * mailbox = clientData->user->mailbox;

    for(int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++){
		if(mailbox->mails[i].deleted){
            char * filepath = mailbox->mails[i].filename;
            char * new_filename = malloc(strlen(filepath) + 1);
            char * fileName = strrchr(filepath, '/');
            sprintf(new_filename, "%s/%s/new/%s", get_maildir(),clientData->username, fileName);
            if(rename(filepath, new_filename) != 0){
                write_error_message(key, INTERNAL_ERROR);
                free(new_filename);
                return;
            }
            mailbox->mails[i].deleted = FALSE;
            mailbox->mails_size += clientData->user->mailbox->mails[i].size;
            mailbox->mail_count++;
            mailbox->deleted_count--;
            free(new_filename);
        }
	}
    write_ok_message(key, JUST_OK);
}

void handle_update_quit(struct selector_key *key){
	client_data * clientData = ATTACHMENT(key);
    t_mailbox * mailbox = clientData->user->mailbox;

    boolean has_message_been_deleted = FALSE;
	for (int i = 0; i < (mailbox->mail_count + mailbox->deleted_count); i++) {
		if (mailbox->mails[i].deleted) {
            has_message_been_deleted = TRUE;
            printf("[POP3] Deleting message %s\n", mailbox->mails[i].filename);
			remove(mailbox->mails[i].filename);
		}
	}
    if(has_message_been_deleted){
        write_ok_message(key, LOGOUT_OUT_MESSAGES_DELETED);
    } else {
        write_ok_message(key, LOGOUT_OUT);
    }
}

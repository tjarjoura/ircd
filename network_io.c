#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <arpa/inet.h>

#include "network_io.h"
#include "client.h"

ssize_t readn(int fd, void* buf, size_t n_bytes)
{
	size_t n_left;
	ssize_t n_read;
	char* cbuf;

	n_left = n_bytes;
	cbuf = buf;

	while (n_left > 0) {
		if ((n_read = read(fd, cbuf, n_left)) < 0) {
			if (errno == EINTR)
				n_read = 0;
			else
				return -1;
		}

		else if (n_read == 0) /* EOF */
			break;

		n_left -= n_read;
		cbuf += n_read;
	}

	return (n_bytes - n_left);
}

ssize_t writen(int fd, void* buf, size_t n_bytes)
{
	size_t n_left;
	ssize_t n_written;
	char* cbuf;

	n_left = n_bytes;
	cbuf = buf;

	while (n_left > 0) {
		if ((n_written = write(fd, cbuf, n_left)) < 0) {
			if (errno == EINTR)
				n_written = 0;
			else
				return -1;
		}

		n_left -= n_written;
		cbuf += n_written;
	}

	return (n_bytes - n_left);
}


int ec_read(int fd, void* buf, size_t n_bytes)
{
	int n;

	if ((n = read(fd, buf, n_bytes)) < 0) {
		fprintf(stderr, "[ERROR] in read(): %s", strerror(errno));
		exit(-1);
	}

	return n;
}

void ec_write(int fd, void* buf, size_t n_bytes)
{
	if (writen(fd, buf, n_bytes) < 0) {
		fprintf(stderr, "[ERROR] in write(): %s\n", strerror(errno));
		exit(-1);
	}
}

void send_message(int conn_fd, int sender_fd, char *message, ...)
{
	char message_buffer[512];
	char sender_buffer[64];
	char content_buffer[448];

	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	va_list ap;

	va_start(ap, message);

	if (sender_fd != -1) { /* relaying a client message */
		if (get_client_prefix(sender_fd, sender_buffer) < 0) {
			printf("Error identifying sender. Invalid file descriptor.\n");
			return;
		}
	} else { /* server is sending its own message */
		getsockname(conn_fd, (struct sockaddr *) &server_addr, &addrlen);
		inet_ntop(AF_INET, &(server_addr.sin_addr), sender_buffer, 56);
	}	

	vsnprintf(content_buffer, 448, message, ap);
	snprintf(message_buffer, 512, ":%s %s\r\n", sender_buffer, content_buffer);
	
	ec_write(conn_fd, message_buffer, strlen(message_buffer));

	va_end(ap);
}	

void send_to_all_visible(struct client *cli, char *message, ...)
{
	int i;
	char message_buffer[448];

	va_list ap;
	va_start(ap, message);

	vsnprintf(message_buffer, 448, message, ap);
	
	send_message(cli->fd, cli->fd, message_buffer);
	for (i = 0; i < MAX_CHAN_JOIN; i++) {
		if (cli->joined_channels[i] != NULL)
			send_to_channel(cli->joined_channels[i], cli->fd, message_buffer);
	}
}

void send_to_channel(struct channel *chan, int cli_fd, char *message, ...)
{
	int i;
	char message_buffer[448];

	va_list ap;
	va_start(ap, message);

	vsnprintf(message_buffer, 448, message, ap);

	for (i = 0; i < MAX_JOIN; i++) {
		if ((chan->joined_users[i] != NULL) && (chan->joined_users[i]->fd != cli_fd))
			send_message(chan->joined_users[i]->fd, cli_fd, message_buffer);
	}
}

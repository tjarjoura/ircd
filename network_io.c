#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

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
	char message_buffer[256];
	char sender_buffer[56];
	char content_buffer[200];

	va_list ap;

	va_start(ap, message);

	if (sender_fd != -1) { /* relay */
		if (get_client_prefix(sender_fd, sender_buffer) < 0) {
			printf("Error identifying sender. Invalid file descriptor.\n");
			return;
		}
	} else
		gethostname(sender_buffer, 50);

	vsnprintf(content_buffer, 200, message, ap);
	snprintf(message_buffer, 256, ":%s %s\r\n", sender_buffer, content_buffer);
	
	ec_write(conn_fd, message_buffer, strlen(message_buffer));

	va_end(ap);
}	

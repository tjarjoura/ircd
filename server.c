#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "command.h"
#include "channel.h"
#include "client.h"
#include "network_io.h"

#define LISTENPORT 6667
#define LISTENMAX 4

static int input_fds[MAX_CLIENTS + 1];
static fd_set input_descriptors;
static int n_fds = 0;

static void initialize()
{
	int i;

	for (i = 0; i < (MAX_CLIENTS + 1); i++)
		input_fds[i] = -1;
	
	initialize_clients();
	initialize_channels();
}

static int add_descriptor(int fd)
{
	int i;

	for (i = 0; i < (MAX_CLIENTS + 1); i++)
		if (input_fds[i] == -1) {
			input_fds[i] = fd;
			if ((fd + 1) > n_fds)
				n_fds = fd + 1;
			break;
		}

	if (i == (MAX_CLIENTS + 1)) {
		fprintf(stderr, "add_descriptor(): Too many connected sockets.\n");
		return -1;
	}

	return 0;
}

static int remove_descriptor(int fd)
{
	int i;

	for (i = 0; i < (MAX_CLIENTS + 1); i++)
		if (input_fds[i] == fd) {
			input_fds[i] = -1;
			break;
		}

	if (i == (MAX_CLIENTS + 1)) {
		fprintf(stderr, "remove_descriptor(): File descriptor not found.\n");
		return -1;
	}

	return 0;
}

static void populate_fd_set()
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (input_fds[i] != -1)
			FD_SET(input_fds[i], &input_descriptors);
	}
}

static int get_listening_socket()
{
	int listen_fd;
	struct sockaddr_in addr;
	int optval = 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(LISTENPORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

	if (bind(listen_fd, (struct sockaddr *)	&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_fd, LISTENMAX) < 0) {
		perror("listen");
		return -1;
	
	}

	return listen_fd;
}

static int parse_args(char *msg, char ***argsp)
{
	int argc, in_arg, trailing, i, j, n;
	char c;

	in_arg = trailing = argc = 0;
	n = strlen(msg);

	for (i = 0; i < n; i++) {
		c = msg[i];
		
		if (c == ':' && !in_arg) {
			trailing = 1;
			argc++;
			in_arg = 1;
		} else if (c == ' ' && in_arg && !trailing) {
			msg[i] = '\0';
			in_arg = 0;
		} else if (c != ' ' && !in_arg) {
			argc++;
			in_arg = 1;
		}
	}

	*(argsp) = malloc((argc + 1) * sizeof(char *));

	i = j = trailing = 0;

	for (j = 0; j < argc; j++) {
		while (msg[i] == ' ')
			i++;
		
		*(*(argsp) + j) = (msg + i);

		while (msg[i] != '\0')
			i++;
		i++;
	}	

	*(*(argsp) + argc) = NULL;

	return argc;
}

static void handle_packet(int cli_fd, char *read_buffer, int n)
{
	/* First see how many commands we got */
	int i, packets = 0;
	int argc; 
	char **args;
	char *bufp;

	for (i = 0; i < n; i++) {
		if (read_buffer[i] == '\r' && read_buffer[i+1] == '\n') {
			packets++;
			read_buffer[i] = read_buffer[i+1] = '\0';
		}
	}

	bufp = read_buffer;
	for (i = 0; i < packets; i++) {
		argc = parse_args(bufp, &args);
		
		if (strcmp(args[0], "QUIT") == 0) {
			remove_client(cli_fd);
			remove_descriptor(cli_fd);
			close(cli_fd);
		} else
			handle_command(cli_fd, argc, args);
		free(args);
		
		/* go to next command */
		while (*bufp != '\0') bufp++;
		while (*bufp == '\0') bufp++;
	}		
}	

int main(int argc, char **argv) 
{
	initialize();
	int sock_fd, conn_fd, listen_fd = get_listening_socket();
	add_descriptor(listen_fd);	
	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	char read_buffer[512];

	int i, n;

	while (1) {
		FD_ZERO(&input_descriptors);
		populate_fd_set();
		
		select(n_fds, &input_descriptors, NULL, NULL, &timeout);

		for (i = 0; i < (MAX_CLIENTS + 1); i++) {
			if ((input_fds[i] != -1) && FD_ISSET(input_fds[i], &input_descriptors)) {
				sock_fd = input_fds[i];

				if (sock_fd == listen_fd) { /* connection request */
					if ((conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL)) < 0)
						perror("accept");
					else {
						add_descriptor(conn_fd);
						new_client(conn_fd);
					}
				} else { /* message sent from client */
					n = ec_read(sock_fd, read_buffer, 512);
					
					if (n == 0) { /* client closed connection */ 
						remove_descriptor(sock_fd);
						remove_client(sock_fd);
						close(sock_fd);
					} else
						handle_packet(sock_fd, read_buffer, n);
				}			
			}	
		}
	}	
	
	return 0;
}


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client.h"

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
}

static int add_descriptor(int fd)
{
	int i;

	for (i = 0; i < (MAX_CLIENTS + 1); i++)
		if (input_fds[i] == -1) {
			input_fds[i] == fd;
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
		return -1
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

	if (bind(listen_fd, (struct sockaddr *)	addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_fd, LISTENMAX) < 0) {
		perror("listen");
		return -1;
	
	}

	return listen_fd;
}

int main(int argc, char **argv) 
{
	initialize();
	int sock_fd, conn_fd, listen_fd = get_listening_socket();
	add_descriptor(listen_fd);	
	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int i;

	while (1) {
		FD_ZERO(&input_descriptors);
		populate_fd_set();
		
		select(n_fds, &input_descriptors, NULL, NULL, timeout);

		for (i = 0; i < (MAX_CLIENTS + 1); i++)
			if ((input_fds[i] != -1) && ISSET(input_fds[i], &input_descriptors)) {
				sock_fd = input_fds[i];

				if (sock_fd == listen_fd) {
					if ((conn_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL)) < 0)
						perror("accept");
					else {
						add_descriptor(conn_fd);
						new_client(conn_fd);		
	}

	return 0;
}

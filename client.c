#include <sys/socket.h>
#include "client.h"

struct client clients[MAX_CLIENTS];
static int n_clients = 0;

void initialize_clients()
{
	int i;

	memset(clients, 0x00, sizeof(struct client) * MAX_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
		clients[i].fd = -1;
}

int new_client(int conn_fd)
{
	int i;

	if (n_clients == MAX_CLIENTS) {
		printf("Error: Max number of clients reached.\n");
		return -1;
	}

	for (i = 0; i < MAX_CLIENTS; i++) 
		if (clients[i].fd == -1)
			break;

	clients[i].sock_fd = conn_fd;
	if (getpeername(conn_fd, &clients[i].peername, sizeof(struct sockaddr_in)) < 0) {
		perror("getpeername");
		return -1;
	}

	n_clients++;
	return 0;
}

int remove_client(int conn_fd)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == conn_fd)
			break;

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", conn_fd);
		return -1;
	}

	memset(clients[i], 0x00, sizeof(struct client));
	clients[i].fd = -1;

	return 0;
}

int set_pass(int conn_fd, char *pass)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == conn_fd) {
			strncpy(clients[i].pass, pass, 20);
			break;
		}

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", conn_fd);
		return -1;
	}

	return 0;
}

int set_nick(int conn_fd, char *nick)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].nick[0] && (strncmp(nick, clients[i].nick, 20) == 0))
			return ERR_NICKNAMEINUSE;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == conn_fd) {
			strncpy(clients[i].nick, nick, 20);
			break;
		}
	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", conn_fd);
		return -1;
	}

	return 0;
}

int set_user(int conn_fd, char *username, int mode, char *realname)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == conn_fd) {
			strncpy(client[i].user, username, 20);
			strncpy(client[i].realname, realname, 30);
			client[i].mode |= mode;
			break;
		}

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", conn_fd);
		return -1;
	}

	return 0;
}

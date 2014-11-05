#include <sys/socket.h>
#include "client.h"

struct client {
	char pass[20];
	char user[20];
	char nick[20];
	char realname[30];
	struct channel* joined_channels[MAX_CHAN];
	int mode;
	int sock_fd;
	struct sockaddr_in peername;
	int registered;
};

struct client clients[MAX_CLIENTS];
static int n_clients = 0;

void initialize_clients()
{
	int i;

	memset(clients, 0x00, sizeof(struct client) * MAX_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
		clients[i].fd = -1;
}

void get_client_prefix(int cli_fd, char *sender_buffer)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)	

int new_client(int cli_fd)
{
	int i;

	if (n_clients == MAX_CLIENTS) {
		printf("Error: Max number of clients reached.\n");
		return -1;
	}

	for (i = 0; i < MAX_CLIENTS; i++) 
		if (clients[i].fd == -1)
			break;

	clients[i].sock_fd = cli_fd;
	if (getpeername(cli_fd, &clients[i].peername, sizeof(struct sockaddr_in)) < 0) {
		perror("getpeername");
		return -1;
	}

	n_clients++;
	return 0;
}

int remove_client(int cli_fd)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == cli_fd)
			break;

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", cli_fd);
		return -1;
	}

	memset(clients[i], 0x00, sizeof(struct client));
	clients[i].fd = -1;

	return 0;
}

int set_pass(int cli_fd, char *pass)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == cli_fd) {
			strncpy(clients[i].pass, pass, 20);
			break;
		}

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", cli_fd);
		return -1;
	}

	return 0;
}

int set_nick(int cli_fd, char *nick)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].nick[0] && (strncmp(nick, clients[i].nick, 20) == 0))
			return ERR_NICKNAMEINUSE;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == cli_fd) {
			strncpy(clients[i].nick, nick, 20);
			break;
		}
	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", cli_fd);
		return -1;
	}

	return 0;
}

int set_user(int cli_fd, char *username, int mode, char *realname)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].fd == cli_fd) {
			strncpy(client[i].user, username, 20);
			strncpy(client[i].realname, realname, 30);
			client[i].mode |= mode;
			break;
		}

	if (i == MAX_CLIENTS) {
		printf("No client for fd %d was found.\n", cli_fd);
		return -1;
	}

	return 0;
}

struct client *get_client(int cli_fd)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	        if (clients[i].fd == cli_fd)
	       		return &clients[i];

	printf("get_client(): No client for file descriptor %d was found\n");
	return NULL;
}

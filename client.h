#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100
#define MAX_CHAN_JOIN 5

struct client {
	char pass[20];
	char user[20];
	char nick[20];
	char realname[30];
	char ip_addr[20];

	struct channel *joined_channels[MAX_CHAN_JOIN];
	int n_joined;
	
	int mode;
	int fd;
	
	int registered;
	int welcomed;
};

extern int n_clients;

void initialize_clients();

int new_client(int conn_fd);
int remove_client(int conn_fd);
int set_pass(struct client *cli, char *pass);
int set_nick(struct client *cli, char *nick);
int set_user(struct client *cli, char *username, int mode, char *realname);

struct client *get_client(int conn_fd);
struct client *get_client_nick(char *nick);

int get_client_prefix(int cli_fd, char *sender_buffer);

#endif

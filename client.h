#ifndef CLIENT_H
#define CLIENT_H

#define MAX_CLIENTS 100

struct client {
	char pass[20];
	char user[20];
	char nick[20];
	char realname[30];
	int mode;
	int sock_fd;
	struct sockaddr_in peername;
};

int new_client(int conn_fd);
int remove_client(int conn_fd);
int set_pass(int conn_fd, char *pass);
int set_nick(int conn_fd, char *nick);
int set_user(int conn_fd, char *username, int mode, char *realname);

extern struct client clients[MAXCLIENTS];

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server.h"
#include "client.h"
#include "replies.h"
#include "network_io.h"
#include "channel.h"

#define NICK_REGISTERED 2
#define USER_REGISTERED 1

struct client clients[MAX_CLIENTS];
static int n_clients = 0;

void initialize_clients()
{
	int i;

	memset(clients, 0x00, sizeof(struct client) * MAX_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
		clients[i].fd = -1;
}

int get_client_prefix(int cli_fd, char *sender_buffer)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd == cli_fd)
			break;
	}

	if (i == MAX_CLIENTS) {
		printf("Error: No client for file descriptor %d was found.\n", cli_fd);
		return -1;
	}
	
	snprintf(sender_buffer, 256, "%s!%s@%s", clients[i].nick, clients[i].user, clients[i].ip_addr);
	return 0;
}

int new_client(int cli_fd)
{
	int i;
	struct sockaddr_in cliaddr;
	unsigned sockaddr_len = sizeof(struct sockaddr_in);

	if (n_clients == MAX_CLIENTS) {
		printf("Error: Max number of clients reached.\n");
		return -1;
	}

	for (i = 0; i < MAX_CLIENTS; i++) 
		if (clients[i].fd == -1)
			break;

	clients[i].fd = cli_fd;
	if (getpeername(cli_fd, (struct sockaddr *) &cliaddr, &sockaddr_len) < 0) {
		perror("getpeername");
		return -1;
	}

	inet_ntop(AF_INET, &(cliaddr.sin_addr), clients[i].ip_addr, 20); 

	clients[i].registered = 0;

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

	memset(&clients[i], 0x00, sizeof(struct client));
	clients[i].fd = -1;

	return 0;
}

int set_nick(struct client *cli, char *nick)
{
	int i;
	char message_buffer[100];

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].nick[0] && (strncmp(nick, clients[i].nick, 20) == 0))
			return ERR_NICKNAMEINUSE;
	
	cli->registered |= NICK_REGISTERED;
	strncpy(cli->nick, nick, 20);

	/* if nick and user are both set, then mark this client as registered */
	if (cli->registered == 3 && !cli->welcomed) {
		send_welcome(cli);
		cli->welcomed = 1;
	} else if (cli->welcomed) {
		snprintf(message_buffer, 100, "NICK %s", nick);
		send_to_all(cli, message_buffer);
	}

	cli->registered |= NICK_REGISTERED;
	strncpy(cli->nick, nick, 20);

	return 0;
}

int set_user(struct client *cli, char *username, int mode, char *realname)
{
	if (cli->registered & USER_REGISTERED)
		return ERR_ALREADYREGISTERED;

	cli->registered |= USER_REGISTERED;
	
	strncpy(cli->user, username, 20);
	
	strncpy(cli->realname, realname, 30);
	cli->mode |= mode;

	/* if nick and user are both set, then mark this client as registered. 3 = both bits set */
	if (cli->registered == 3 && !cli->welcomed) {
		send_welcome(cli);
		cli->welcomed = 1;
	}

	return 0;
}

void send_welcome(struct client *cli)
{
	char addr_buffer[30];

	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	getsockname(cli->fd, (struct sockaddr *) &server_addr, &addrlen);

	inet_ntop(AF_INET, &(server_addr.sin_addr), addr_buffer, 30);

	send_message(cli->fd, -1, "%03d %s :Welcome to the Internet Relay Network %s!%s@%s", RPL_WELCOME, cli->nick, cli->nick, cli->user, cli->ip_addr);
	send_message(cli->fd, -1, "%03d %s :Your host is %s, running version %s", RPL_YOURHOST, cli->nick, addr_buffer, server_version); 
	send_message(cli->fd, -1, "%03d %s :This server was created %s", RPL_CREATED, cli->nick, ctime(&server_start_time));
	send_message(cli->fd, -1, "%03d %s :%s %s <NO MODES>", RPL_MYINFO, cli->nick, addr_buffer, server_version);
	send_message(cli->fd, -1, "%d %s :There are %d users and %d invisible on %d server\n", RPL_LUSERCLIENT, cli->nick, n_clients, 0, 1);
	send_message(cli->fd, -1, "%d %s :-%s Message of the day-", RPL_MOTDSTART, cli->nick, addr_buffer);
	send_message(cli->fd, -1, "%d %s :-%s", RPL_MOTD, cli->nick, motd);
	send_message(cli->fd, -1, "%d %s :End of /MOTD command", RPL_ENDOFMOTD, cli->nick);
}

/* get client by file descriptor */
struct client *get_client(int cli_fd)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	        if (clients[i].fd == cli_fd)
	       		return &clients[i];

	return NULL;
}

/* get client by nick name */
struct client *get_client_nick(char *nick)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (strncmp(clients[i].nick, nick, 20) == 0)
			return &clients[i];
	}

	return NULL;
}

void send_to_all(struct client *cli, char *message)
{
	int i;
	
	send_message(cli->fd, cli->fd, message);
	for (i = 0; i < MAX_CHAN_JOIN; i++) {
		if (cli->joined_channels[i] != NULL)
			relay_message(cli->joined_channels[i], cli->fd, message);
	}
}

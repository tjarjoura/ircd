#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "command.h"
#include "global_stat.h"
#include "client.h"
#include "replies.h"
#include "network_io.h"
#include "channel.h"

#define NICK_REGISTERED 2
#define USER_REGISTERED 1

struct client clients[MAX_CLIENTS];
int n_clients = 0;

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
	struct client *cli = get_client(cli_fd);

	for (i = 0; i < MAX_CHAN_JOIN; i++) {
		if (cli->joined_channels[i] != NULL)
			part_user(cli->joined_channels[i], cli);
	}

	memset(cli, 0x00, sizeof(struct client));
	cli->fd = -1;

	return 0;
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

/* get properly formatted prefix of client(sender) information */
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

static int is_erroneus(char *nick)
{
	int i;

	if (isdigit(nick[0]))
		return 1;

	char allowed[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-\\[]{}^|";

	for (i = 0; nick[i] != '\0'; i++) {
		if (!strchr(allowed, nick[i]))
			return 1;
	}

	return 0;
}

static void set_nick(int fd, int argc, char **args)
{
	int i;
	struct client *cli = get_client(fd);

	if (argc < 2) {
		send_message(fd, -1, "%d :No nickname was given", ERR_NONICKNAMEGIVEN);
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].nick[0] && (strncmp(args[1], clients[i].nick, 20) == 0)) {
			send_message(fd, -1, "%d %s :Nickname is already in use", ERR_NICKNAMEINUSE, args[1]);
			return;
		}
	}

	if (is_erroneus(args[1])) {
		send_message(fd, -1, "%d %s :Erroneous nickname", ERR_ERRONEUSNICKNAME, args[1]);
		return;
	}
	
	cli->registered |= NICK_REGISTERED;

	if (cli->welcomed) 
		send_to_all_visible(cli, "NICK %s", args[1]);

	strncpy(cli->nick, args[1], 20);
	
	/* if nick and user are both set, then mark this client as registered */
	if (cli->registered == 3 && !cli->welcomed) {
		send_welcome(cli);
		cli->welcomed = 1;
	}
}

static void set_user(int fd, int argc, char **args)
{
	struct client *cli = get_client(fd);

	if (argc < 5) {
		send_message(fd, -1, "%d %s :Not enough parameters", ERR_NEEDMOREPARAMS, args[0]); 
		return;
	}

	if (cli->registered & USER_REGISTERED) {
		send_message(fd, -1, "%d :Unauthorized command (already registered", ERR_ALREADYREGISTERED);
		return;
	}

	cli->registered |= USER_REGISTERED;
	
	strncpy(cli->user, args[1], 20);
	
	strncpy(cli->realname, args[4], 30);
	cli->mode |= atoi(args[2]);

	/* if nick and user are both set, then mark this client as registered. 3 = both bits set */
	if (cli->registered == 3 && !cli->welcomed) {
		send_welcome(cli);
		cli->welcomed = 1;
	}
}

void initialize_clients()
{
	int i;

	memset(clients, 0x00, sizeof(struct client) * MAX_CLIENTS);

	for (i = 0; i < MAX_CLIENTS; i++)
		clients[i].fd = -1;

	register_command("USER", set_user);
	register_command("NICK", set_nick);
}

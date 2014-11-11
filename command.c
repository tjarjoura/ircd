#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "network_io.h"
#include "client.h"
#include "channel.h"
#include "replies.h"

struct command {
	const char *cmd;
	void (*cmd_cb)(int fd, int argc, char **args);
};

static void handle_ping(int fd, int argc, char **args);
static void handle_nick(int fd, int argc, char **args);
static void handle_user(int fd, int argc, char **args);
static void handle_join(int fd, int argc, char **args);
static void handle_part(int fd, int argc, char **args);
static void handle_privmsg(int fd, int argc, char **args);

static struct command commands[] = {
				  {.cmd = "PING", .cmd_cb = handle_ping},
				  {.cmd = "NICK", .cmd_cb = handle_nick},
				  {.cmd = "USER", .cmd_cb = handle_user},
				  {.cmd = "JOIN", .cmd_cb = handle_join},
				  {.cmd = "PART", .cmd_cb = handle_part},
				  {.cmd = "PRIVMSG", .cmd_cb = handle_privmsg}
};

static int n_commands = 6;

void handle_command(int fd, int argc, char **args)
{
	int i;
	
	struct client *cli = get_client(fd);

	for (i = 0; i < n_commands; i++) {
		if (strcmp(args[0], commands[i].cmd) == 0) {
			commands[i].cmd_cb(fd, argc, args);
			break;
		}
	}

	if (i == n_commands)
		send_message(fd, -1, "%d %s %s :Unknown command", ERR_UNKNOWNCOMMAND, cli->nick, args[0]);
}	

static void handle_ping(int fd, int argc, char **args)
{
	char addr_buffer[30];

	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	getsockname(fd, (struct sockaddr *) &server_addr, &addrlen);

	inet_ntop(AF_INET, &(server_addr.sin_addr), addr_buffer, 30);

	send_message(fd, -1, "PONG %s", addr_buffer); 	
}

static void handle_nick(int fd, int argc, char **args)
{
	int rv;
	struct client *cli = get_client(fd);
		

	if (argc < 2)
		rv = ERR_NONICKNAMEGIVEN;
	else if (cli == NULL)
		return;
	else
		rv = set_nick(cli, args[1]);

	switch (rv) {
		case ERR_ERRONEUSNICKNAME:
			send_message(fd, -1, "%d %s %s :Erroneus nickname", ERR_ERRONEUSNICKNAME, cli->nick, args[1]);
			break;
		case ERR_NONICKNAMEGIVEN:
			send_message(fd, -1, "%d %s :No nickname given", ERR_NONICKNAMEGIVEN, cli->nick);
			break;
		case ERR_NICKNAMEINUSE:
			send_message(fd, -1, "%d %s %s :Nickname is already in use", ERR_NICKNAMEINUSE, cli->nick, args[1]);
			break;
	}
}

static void handle_user(int fd, int argc, char **args)
{
	int rv;
	struct client *cli = get_client(fd);

	if (argc < 5)
		rv = ERR_NEEDMOREPARAMS;

	rv = set_user(cli, args[1], atoi(args[2]), args[4]); 	

	switch (rv) {
		case ERR_NEEDMOREPARAMS:
			send_message(fd, -1, "%d %s %s :Not enough parameters", ERR_NEEDMOREPARAMS, cli->nick, args[0]);
			break;
		case ERR_ALREADYREGISTERED:
			send_message(fd, -1, "%d %s :Unauthorized command (already registered)", ERR_ALREADYREGISTERED, cli->nick);
			break;
	}
}

static void handle_join(int fd, int argc, char **args)
{
	struct client *cli = get_client(fd);
	struct channel *chan;
	
	if (argc < 2) {
		send_message(fd, -1, "%d %s %s :Not enough parameters", ERR_NEEDMOREPARAMS, cli->nick, args[0]);
		return;
	}

	char *bufp = args[1];
	int i, n = 1;

	while (*bufp != '\0') {
		if (*bufp == ',') {
			*bufp = '\0';
			n++;
		}
	
		bufp++;
	}

	bufp = args[1];
	for (i = 0; i < n; i++) {
		if ((chan = get_channel(bufp)) == NULL) {
			if ((chan = new_channel(bufp)) == NULL) {
				send_message(fd, -1, "%d %s %s :No such channel/Too many channels", ERR_NOSUCHCHANNEL, cli->nick, args[0]);
				return;
			}
		}

		/* Must be in this order */
		send_message(cli->fd, cli->fd, "JOIN %s", bufp);
		join_channel(chan, cli);
		send_to_channel(chan, cli->fd, "JOIN %s", bufp);

		/* advance to next channel name */
		while (*bufp != '\0') bufp++;
		bufp++;
	}
}

static void handle_part(int fd, int argc, char **args)
{
	struct client *cli = get_client(fd);
	struct channel *chan;

	if (argc < 2) {
		send_message(fd, -1, "%d %s %s :Not enough parameters", ERR_NEEDMOREPARAMS, cli->nick, args[0]);
		return;
	}

	char *bufp = args[1];
	int i, n = 1;

	while (*bufp != '\0') {
		if (*bufp == ',') {
			n++;
			*bufp = '\0';
		}

		bufp++;
	}

	bufp = args[1];

	for (i = 0; i < n; i++) {
		if ((chan = get_channel(bufp)) == NULL) {
			send_message(fd, -1, "%d %s %s :No such channel", ERR_NOSUCHCHANNEL, cli->nick, args[0]);
			return;
		}

		part_user(chan, cli);
		
		send_to_channel(chan, cli->fd, "PART %s %s", chan->name, args[1]);

		/* advance to next channel name */
		while (*bufp != '\0') bufp++;
		bufp++;
	}
}

static void handle_privmsg(int fd, int argc, char **args)
{
	struct client  *cli = get_client(fd);
	struct channel *target_chan;
	struct client *target_cli;

	char *bufp = args[1];
	int i, n = 1;
	int is_channel = 1;

	if (argc < 3) {
		send_message(fd, -1, "%d %s :No text to send", ERR_NOTEXTTOSEND, cli->nick);
		return;
	}

	if (argc < 2) {
		send_message(fd, -1, "%d %s :No recipient given (PRIVMSG)", ERR_NORECIPIENT, cli->nick);
		return;
	}

	while (*bufp != '\0') {
		if (*bufp == ',') {
			n++;
			*bufp = '\0';
		}
		
		bufp++;
	}

	bufp = args[1];

	for (i = 0; i < n; i++) {
		if ((target_chan = get_channel(bufp)) == NULL) {
			if ((target_cli = get_client_nick(bufp)) == NULL) {
				send_message(fd, -1, "%d %s %s :No such nick/channel", ERR_NOSUCHNICK, cli->fd, bufp);
				continue;
			}

			is_channel = 0;
		}

		if (is_channel && !in_channel(target_chan, cli->fd)) {
			send_message(fd, -1, "%d %s :Cannot send to channel", ERR_CANNOTSENDTOCHAN, bufp);
			continue;
		}

		is_channel ? send_to_channel(target_chan, cli->fd, "PRIVMSG %s %s", bufp, args[2]) : send_message(target_cli->fd, cli->fd, "PRIVMSG %s %s", bufp, args[2]);
	}
}	

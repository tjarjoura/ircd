#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "network_io.h"
#include "client.h"
#include "channel.h"
#include "replies.h"

struct command {
	const char *cmd;
	void (*cmd_cb)(int fd, int argc, char **args);
};

static void handle_pass(int fd, int argc, char **args);
static void handle_nick(int fd, int argc, char **args);
static void handle_user(int fd, int argc, char **args);
static void handle_join(int fd, int argc, char **args);
static void handle_part(int fd, int argc, char **args);

static struct command commands[] = {
				  {.cmd = "PASS", .cmd_cb = handle_pass},
				  {.cmd = "NICK", .cmd_cb = handle_nick},
				  {.cmd = "USER", .cmd_cb = handle_user},
				  {.cmd = "JOIN", .cmd_cb = handle_join},
				  {.cmd = "PART", .cmd_cb = handle_part}
};

static int n_commands = 5;

void handle_command(int fd, int argc, char **args)
{
	int i;

	for (i = 0; i < n_commands; i++) {
		if (strcmp(args[0], commands[i].cmd) == 0) {
			commands[i].cmd_cb(fd, argc, args);
			break;
		}
	}

	if (i == n_commands)
		send_message(fd, -1, "%d %s :Unknown command", ERR_UNKNOWNCOMMAND, args[0]);
}	

static void handle_pass(int fd, int argc, char **args) 
{
	int rv;

	if (argc < 2)
		rv = ERR_NEEDMOREPARAMS;
	else
		rv = set_pass(fd, args[1]);
	
	switch (rv) {
		case ERR_NEEDMOREPARAMS:
			send_message(fd, -1, "%d %s :Not enough parameters", ERR_NEEDMOREPARAMS, args[0]);
			break;
	}
}

static void handle_nick(int fd, int argc, char **args)
{
	int rv;

	if (argc < 2)
		rv = ERR_NONICKNAMEGIVEN;
	else
		rv = set_nick(fd, args[1]);

	switch (rv) {
		case ERR_ERRONEUSNICKNAME:
			send_message(fd, -1, "%d %s :Erroneus nickname", ERR_ERRONEUSNICKNAME, args[1]);
			break;
		case ERR_NONICKNAMEGIVEN:
			send_message(fd, -1, "%d :No nickname given", ERR_NONICKNAMEGIVEN);
			break;
		case ERR_NICKNAMEINUSE:
			send_message(fd, -1, "%d %s :Nickname is already in use", ERR_NICKNAMEINUSE, args[1]);
			break;
	}
}

static void handle_user(int fd, int argc, char **args)
{
	int rv;
	
	if (argc < 5)
		rv = ERR_NEEDMOREPARAMS;

	rv = set_user(fd, args[1], atoi(args[2]), args[4]); 	

	switch (rv) {
		case ERR_NEEDMOREPARAMS:
			send_message(fd, -1, "%d %s :Not enough parameters", ERR_NEEDMOREPARAMS, args[0]);
			break;
		case ERR_ALREADYREGISTERED:
			send_message(fd, -1, "%d :Unauthorized command (already registered)", ERR_ALREADYREGISTERED);
			break;
	}
}

static void handle_join(int fd, int argc, char **args)
{
	if (argc < 2) {
		send_message(fd, -1, "%d %s :Not enough parameters", ERR_NEEDMOREPARAMS, args[0]);
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
		printf("calling join_channel(%s, %d)\n", bufp, fd);
		join_channel(bufp, fd);
		
		/* advance to next channel name */
		while (*bufp != '\0') bufp++;
		bufp++;
	}
}

static void handle_part(int fd, int argc, char **args)
{
	if (argc < 2) {
		send_message(fd, -1, "%d %s :Not enough parameters", ERR_NEEDMOREPARAMS, args[0]);
		return;
	}

	char *bufp = args[1];
	int i, n = 0;

	while (*bufp != '\0') {
		if (*bufp == ',') {
			n++;
			*bufp = '\0';
		}

		bufp++;
	}

	bufp = args[1];

	for (i = 0; i < n; i++) {
		part_user(bufp, fd);

		/* advance to next channel name */
		while (*bufp != '\0') bufp++;
		bufp++;
	}
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "network_io.h"
#include "client.h"
#include "channel.h"
#include "replies.h"
#include "command.h"

#define MAX_COMMANDS 25

struct command {
	char cmd[10];
	void (*cmd_cb)(int fd, int argc, char **args);
};

static struct command commands[MAX_COMMANDS]; 
static int n_commands = 0;

void register_command(char *cmd_name, void (*callback)(int fd, int argc, char **args))
{
	int i;

	for (i = 0; i < n_commands; i++) {
		if (strncmp(commands[i].cmd, cmd_name, 10) == 0)
			break;
	}

	if (i == n_commands) { /* adding a new command */
		strncpy(commands[i].cmd, cmd_name, 10);
		n_commands++;
	}

	commands[i].cmd_cb = callback;
}

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

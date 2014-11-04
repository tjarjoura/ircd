#include "network_io.h"
#include "client.h"
#include "errors.h"

struct command {
	char cmd[10];
	void (*cmd_cb)(int fd, int argc, char **args);
};

static void handle_pass(int fd, int argc, char **args);
static void handle_nick(int fd, int argc, char **args);
static void handle_user(int fd, int argc, char **args);

static struct command commands = {
				  {.cmd = "PASS", .cmd_cb = handle_pass},
				  {.cmd = "NICK", .cmd_cb = handle_nick},
				  {.cmd = "USER", .cmd_cb = handle_user}
};

static int n_commands = 3;

void handle_command(int fd, int argc, char **args)
{
	int i;

	for (i = 0; i < n_commands; i++) {
		if (strcmp(args[0], commands[i].cmds) == 0) {
			commands[i].cmd_cb(fd, argc, args);
			break;
		}
	}
}	

static int handle_pass(int fd, int argc, char **args) 
{
	int i;

	if (argc < 2)
		return ERR_NEEDMOREPARAMS;

	return set_pass(fd, args[1]);
}

static int handle_nick(int fd, int argc, char **args)
{
	int i;

	if (argc < 2)
		ERR_NONICKNAMEGIVEN;

	return set_nick(fd, args[1]);
}

static int handle_user(int fd, int argc, char **args)
{
	int i;
	
	if (argc < 5)
		ERR_NEEDMOREPARAMS;
	
	return set_user(fd, args[1], args[2], args[4]); 	

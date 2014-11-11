#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "network_io.h"
#include "channel.h"
#include "channel_stat.h"
#include "client.h"
#include "replies.h"

struct channel channels[MAX_CHANNELS];

static int n_channels = 0;

int in_channel(struct channel *chan, int cli_fd)
{
	int i;

	for (i = 0; i < MAX_JOIN; i++) {
		if ((chan->joined_users[i] != NULL) && (chan->joined_users[i]->fd == cli_fd))
			return 1;
	}

	return 0;
}

struct channel *get_channel(char *chan_name)
{
	int i;

	for (i = 0; i < MAX_CHANNELS; i++)
		if (strncmp(channels[i].name, chan_name, 20) == 0)
			break;

	if (i == MAX_CHANNELS)
		return NULL;
	
	return &(channels[i]);
}

struct channel *new_channel(char *chan_name)
{
	int i;

	for (i = 0; i < MAX_CHANNELS; i++) {
		if (channels[i].n_joined == -1) /* -1 users joined means this slot in the channel array is open */
			break;
	}

	if (i == MAX_CHANNELS)
		return NULL;

	strncpy(channels[i].name, chan_name, 20);
	channels[i].n_joined = 0;

	n_channels++;

	return &(channels[i]);
}

static void remove_inactive_channels(int n)
{
	int n_removed = 0;
	int i;

	for (i = 0; i < MAX_CHANNELS; i++) {
		if (n_removed == n)
			break;
		if (channels[i].n_joined == 0) {
			memset(&channels[i], 0x00, sizeof(struct channel));
			channels[i].n_joined = 1;
			n_removed++;
		}
	}
}

static void join_channel(struct channel *chan, struct client *cli)
{
	int i, j;

	if (cli->n_joined == MAX_CHAN_JOIN) {
	        send_message(cli->fd, -1, "%d %s %s :You have joined too many channels", ERR_TOOMANYCHANNELS, cli->nick, chan->name);
       		return;
	}	

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i] == NULL) {
			chan->joined_users[i] = cli;
			chan->n_joined++;	

			/* add to clients joined channel list */
			for (j = 0; j < MAX_CHAN_JOIN; j++) {
				if (cli->joined_channels[j] == NULL) {
					cli->joined_channels[j] = chan;
					cli->n_joined++;
					break;
				}
			}

			break;
		}
	}

	if (i == MAX_JOIN) {
		send_message(cli->fd, -1, "%d %s %s :Cannot join channel (+l)", ERR_CHANNELISFULL, cli->nick, chan->name);
		return;
	}

	send_channel_greeting(chan, cli);
}

void part_user(struct channel *chan, struct client *cli)
{
	int i;

	/* find the spot in the joined users array where the client is */
	for (i = 0; i < MAX_JOIN; i++) {
		if((chan->joined_users[i] != NULL) && (cli->fd == chan->joined_users[i]->fd))
			break;
	}

	if (i == MAX_JOIN) {
		send_message(cli->fd, -1, "%d %s %s :You are not on that channel", ERR_NOTONCHANNEL, cli->nick, chan->name);
		return;	
	}

	chan->joined_users[i] = NULL;
	chan->n_joined--;

	/* remove channel from the client list of joined_channels */
	for (i = 0; i < MAX_CHAN_JOIN; i++) {
		if (strcmp(cli->joined_channels[i]->name, chan->name) == 0) {
			cli->joined_channels[i] = NULL;
			cli->n_joined--;
			break;
		}
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

	while (*bufp != '\0') { /* could be multiple channels passed to one command, they should be comma-separated */
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
		
		if (in_channel(chan, cli->fd)) /* can't join a channel twice, but IRC specifies no error code for this */
			continue;

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

		send_message(cli->fd, cli->fd, "PART %s %s", bufp, args[2]);
		part_user(chan, cli);
		send_to_channel(chan, cli->fd, "PART %s %s", chan->name, args[2]);

		/* advance to next channel name */
		while (*bufp != '\0') bufp++;
		bufp++;
	}
}

void initialize_channels()
{
	int i;

	memset(&channels, 0x00, sizeof(struct channel) * MAX_CHANNELS);

	for (i = 0; i < MAX_CHANNELS; i++)
		channels[i].n_joined = -1;

	register_command("JOIN", handle_join);
	register_command("PART", handle_part);
}

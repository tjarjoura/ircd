#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_io.h"
#include "channel.h"
#include "client.h"
#include "replies.h"

struct channel channels[MAX_CHANNELS];

static int n_channels = 0;

void initialize_channels()
{
	int i;

	memset(&channels, 0x00, sizeof(struct channel) * MAX_CHANNELS);

	for (i = 0; i < MAX_CHANNELS; i++)
		channels[i].n_joined = -1;
}

int in_channel(struct channel *chan, int cli_fd)
{
	int i;

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i]->fd == cli_fd)
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

static void send_channel_greeting(struct channel *chan, struct client *cli)
{
	char user_list[400];
	
	int i, n = 0;

	if (chan->topic[0] != '\0') 
		send_message(cli->fd, -1, "%d %s %s: %s", RPL_TOPIC, cli->nick, chan->name, chan->topic);	

	memset(user_list, 0x00, 400);

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i] != NULL) {
			strncat(user_list, chan->joined_users[i]->nick, 20);
			strncat(user_list, " ", 1);
			n += strlen(chan->joined_users[i]->nick + 1);
			
			if (n >= 379) { /* if maximum capacity reached, send what we have so far */
				send_message(cli->fd, -1, "%d %s = %s :%s", RPL_NAMREPLY, cli->nick, chan->name, user_list);
				memset(user_list, 0x00, 400);
				n = 0;
			}
		}
	}

	if (n > 0)
		send_message(cli->fd, -1, "%d %s = %s :%s", RPL_NAMREPLY, cli->nick, chan->name, user_list);

	send_message(cli->fd, -1, "%d %s %s :End of /NAMES list", RPL_ENDOFNAMES, cli->nick, chan->name);
}

void relay_message(struct channel *chan, int cli_fd, char *message)
{
	int i;

	for (i = 0; i < MAX_JOIN; i++) {
		if ((chan->joined_users[i] != NULL) && (chan->joined_users[i]->fd != cli_fd))
			send_message(chan->joined_users[i]->fd, cli_fd, message);
	}
}

void join_channel(struct channel *chan, struct client *cli)
{
	int i, j;

	if (cli->n_joined == MAX_CHAN_JOIN) {
	        send_message(cli->fd, -1, "%d %s %s :You have joined too many channels", ERR_TOOMANYCHANNELS, cli->nick, chan->name);
       		return;
	}	

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i] == NULL) {
			chan->joined_users[i] = cli;
			
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

	/* remove channel from the client list of joined_channels */
	for (i = 0; i < MAX_CHAN_JOIN; i++) {
		if (strcmp(cli->joined_channels[i]->name, chan->name) == 0) {
			cli->joined_channels[i] = NULL;
			break;
		}
	}
}	

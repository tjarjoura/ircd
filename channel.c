#include <stdlib.h>
#include "network_io.h"
#include "channel.h"
#include "client.h"
#include "replies.h"

struct channel {
	char name[20];
	char topic[100];
	struct client *joined_users[MAX_JOIN];
	int n_joined; 
	unsigned int mode; /*bit mask representing private/secret/invite-only/topic/moderated/ */
};

struct channel channels[MAX_CHANNELS];

static int n_channels = 0;

void initialize_channels()
{
	int i;

	memset(&channels, 0x00, sizeof(struct channel) * MAX_CHANNELS);

	for (i = 0; i < MAX_CHANNELS; i++)
		channels[i].n_joined = -1;
}

static int add_channel(char *chan_name)
{
	int i;

	for (i = 0; i < MAX_CHANNELS; i++) {
		if (channels[i].n_joined == -1)
			break;
	}

	if (i == MAX_CHANNELS)
		return -1;

	channels[i].used = 1;
	strncpy(channels[i].name, chan_name, 20);

	n_channels++;

	return i;
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

static void relay_user_quit(struct channel *chan, int cli_fd, char *quit_message)
{
	int i;

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i] != NULL)
			send_message(chan->joined_users[i]->fd, cli_fd, "QUIT :%s", quit_message);
	}
}

static void send_channel_greeting(struct channel *chan, int cli_fd)
{
	char topic[400];
	char user_list[400];
	
	int i, n = 0;

	if (chan->topic[0] != '\0') 
		send_message(cli_fd, -1, "%d %s: %s", RPL_TOPIC, chan->name, chan->topic);	

	memset(user_list, 0x00, 400);

	for (i = 0; i < MAX_JOIN; i++) {
		if (chan->joined_users[i] != NULL) {
			strncat(user_list, chan->joined_users[i]->nick, 20);
			n += strlen(chan->joined_users[i]->nick);
			
			if (n >= 379) { /* if maximum capacity reached, send what we have so far */
				send_message(cli_fd, -1, "%d = %s :%s", RPL_NAMEREPLY, chan->name, user_list);
				memset(user_list, 0x00, 400);
				n = 0;
			}
		}
	}

	send_message(cli_fd, -1, "%d %s :End of /NAMES list", RPL_ENDOFNAMES, chan->name);
}

void join_channel(char *chan_name, int cli_fd)
{
	int i, j, k;

	for (i = 0; i < MAX_CHANNELS; i++) {
		if (strcmp(channel[i].name, chan_name) == 0)
			break;
	}

	if (i == MAX_CHANNELS)
		if ((i = add_channel(chan_name)) < 0) {
			send_message(cli_fd, -1, "%d %s :No such channel", ERR_NOSUCHCHANNEL, chan_name);
			return;
		}
	
	struct client *cli = get_client(cli_fd);

	if (cli->n_joined == MAX_CHAN_JOIN) {
	        send_message(cli_fd, -1, "%d %s :You have joined too many channels", ERR_TOOMANYCHANNELS, chan_name);
       		return;
	}	

	for (j = 0; j < MAX_JOIN; j++) {
		if (channels[i].joined_users[j] == NULL) {
			channels[i].joined_users[j] = cli;
			
			/* add to clients joined channel list */
			for (k = 0; k < MAX_CHAN_JOIN; k++) {
				if (cli->joined_channels[k] == NULL) {
					cli->joined_channels[k] = &channels[i];
					cli->n_joined++;
				}
			}
		}
	}

	if (j == MAX_JOIN)
		send_message(cli_fd, -1, "%d %s :Cannot join channel (+l)", ERR_CHANNELISFULL, chan_name);

	send_channel_greeting(&channels[i], cli_fd);

	/* relay JOIN message */
	for (j = 0; j < MAX_JOIN; j++) {
	       if (channels[i].joined_users[j] != NULL)
	       		send_message(channels[i].joined_users[j]->fd, cli_fd, "JOIN %s", chan_name);	       
	}
}

void part_user(char *chan_name, int cli_fd)
{
	int i, j;
	struct client *cli;

	for (i = 0; i < MAX_CHANNELS; i++) {
		if (strcmp(channel[i].name, chan_name) == 0)
			break;
	}

	if (i == MAX_CHANNELS) {
		send_message(cli_fd, -1, "%d %s :No such channel", ERR_NOSUCHCHANNEL, chan_name);
		return;
	}

	for (j = 0; j < MAX_JOIN; i++) {
		if((channels[i].joined_users[j] != NULL) && (cli_fd == channels[i].joined_users[j]))
			break;
	}

	if (j == MAX_JOIN) {
		send_message(cli_fd, -1, "%d %s :You are not on that channel", ERR_NOTONCHANNEL, chan_name);
		return;	
	}

	channels[i].joined_users[j] = NULL;

	cli = get_client(cli_fd);

	for (j = 0; j < MAX_CHAN_JOIN; j++) {
		if (strcmp(cli->joined_channels[j]->name, chan_name) == 0) {
			cli->joined_channels[j] = NULL;
			break;
		}
	}

	/* rellayyyyy */
	for (j = 0; j < MAX_JOIN; j++) {
	       if (channels[i].joined_users[j] != NULL)
			send_message(channels[i].joined_users[j]->fd, cli_fd, "PART %s", chan_name);
	}	    
}	

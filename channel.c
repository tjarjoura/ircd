#include <stdlib.h>
#include "network_io.h"
#include "channel.h"
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

void join_channel(char *chan_name, int cli_fd)
{
	int i, j;

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

	for (j = 0; j < MAX_JOIN; j++) {
		if (channels[i].joined_users[j] == NULL)
			channels[i].joined_users[j] = cli;

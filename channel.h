#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_CHANNELS 40
#define MAX_JOIN 20

struct channel {
	char name[20];
	struct client *joined_users[MAX_JOIN];
	int n_joined; 
	unsigned int mode; /*bit mask representing private/secret/invite-only/topic/moderated/ */

	char topic[400];
	char topic_setter[20];
	time_t topic_set_time;
};

void initialize_channels();

struct channel *new_channel(char *chan_name);
struct channel *get_channel(char *chan_name);

int in_channel(struct channel *chan, int cli_fd);

void part_user(struct channel *chan, struct client *cli);

#endif

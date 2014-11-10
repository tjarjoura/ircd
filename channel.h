#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_CHANNELS 40
#define MAX_JOIN 20

struct channel {
	char name[20];
	char topic[100];
	struct client *joined_users[MAX_JOIN];
	int n_joined; 
	unsigned int mode; /*bit mask representing private/secret/invite-only/topic/moderated/ */
};

void initialize_channels();

struct channel *new_channel(char *chan_name);
struct channel *get_channel(char *chan_name);

void join_channel(struct channel *chan, struct client *cli);
void part_user(struct channel *chan, struct client *cli);
int kick_user(char *chan_name, int cli_fd); 
int set_topic(char* chan_name, int cli_fd);
void relay_message(struct channel *chan, int cli_fd, char *message);

int in_channel(struct channel *chan, int cli_fd);
#endif

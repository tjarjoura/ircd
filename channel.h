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

void join_channel(char *chan_name, int cli_fd);
void part_user(char *chan_name, int cli_fd);
int kick_user(char *chan_name, int cli_fd); 
int set_topic(char* chan_name, int cli_fd);
void relay_user_quit(struct channel *chan, int cli_fd, char *quit_message);
void relay_user_message(char *chan_name, int cli_fd, char *message);

#endif

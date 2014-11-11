#include <string.h>

#include "channel_stat.h"
#include "network_io.h"
#include "replies.h"

void set_topic(struct channel *chan, char *topic)
{
	return;
}

void send_channel_greeting(struct channel *chan, struct client *cli)
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

	if (n > 0) {
		send_message(cli->fd, -1, "%d %s = %s :%s", RPL_NAMREPLY, cli->nick, chan->name, user_list);
		memset(user_list, 0x00, 400);
	}

	send_message(cli->fd, -1, "%d %s %s :End of /NAMES list", RPL_ENDOFNAMES, cli->nick, chan->name);
}

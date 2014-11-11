#include <string.h>
#include <time.h>

#include "channel_stat.h"
#include "channel.h"
#include "command.h"
#include "network_io.h"
#include "replies.h"

static void handle_topic(int fd, int argc, char **args)
{
	struct channel *chan;
	struct client *cli = get_client(fd);

	if (argc < 3) { /* Show topic */
		if (argc < 2) {
			send_message(fd, -1, "%d %s %s :Not enough parameters", ERR_NEEDMOREPARAMS, cli->nick, args[0]);
			return;
		}
	
		chan = get_channel(args[1]);

		if (!in_channel(chan, fd)) {
			send_message(fd, -1, "%d %s %s :You are not on that channel", ERR_NOTONCHANNEL, cli->nick, args[1]);
			return;

		}

		if (chan->topic_set_time == 0) {
			send_message(fd, -1, "%d %s %s :Topic not set", RPL_NOTOPIC, cli->nick, args[1]);
			return;
		}

		send_message(fd, -1, "%d %s %s %s", RPL_TOPIC, cli->nick, args[1], chan->topic);
		send_message(fd, -1, "%d %s %s %s %d", RPL_TOPICWHOTIME, cli->nick, args[1], chan->topic_setter, chan->topic_set_time);
		return;
	}

	chan = get_channel(args[1]);

	/* View topic */
	send_message(cli->fd, cli->fd, "TOPIC %s %s", args[1], args[2]);
	send_to_channel(chan, cli->fd, "TOPIC %s %s", args[1], args[2]);

	if (strlen(args[2]) > 1) { /* one character for ':' */
		chan->topic_set_time = time(NULL);
		strncpy(chan->topic_setter, cli->nick, 20);
		strncpy(chan->topic, args[2], 400);
	} else { /* unset topic */
		chan->topic_set_time = 0;
	}
}

void send_channel_greeting(struct channel *chan, struct client *cli)
{
	char user_list[400];
	
	int i, n = 0;

	if (chan->topic_set_time != 0) {
		send_message(cli->fd, -1, "%d %s %s %s", RPL_TOPIC, cli->nick, chan->name, chan->topic);	
		send_message(cli->fd, -1, "%d %s %s %s %d", RPL_TOPICWHOTIME, cli->nick, chan->name, chan->topic_setter, chan->topic_set_time);
	}

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

void initialize_channel_stat()
{
	register_command("TOPIC", handle_topic);
}

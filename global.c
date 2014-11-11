#include <arpa/inet.h>

#include "command.h"
#include "client.h"
#include "global.h"
#include "network_io.h"
#include "replies.h"

static void handle_ping(int fd, int argc, char **args)
{
	char addr_buffer[30];

	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr);

	getsockname(fd, (struct sockaddr *) &server_addr, &addrlen);

	inet_ntop(AF_INET, &(server_addr.sin_addr), addr_buffer, 30);

	send_message(fd, -1, "PONG %s", addr_buffer); 	
}

static void handle_privmsg(int fd, int argc, char **args)
{
	struct client  *cli = get_client(fd);
	struct channel *target_chan;
	struct client *target_cli;

	char *bufp = args[1];
	int i, n = 1;
	int is_channel = 1;

	if (argc < 3) {
		send_message(fd, -1, "%d %s :No text to send", ERR_NOTEXTTOSEND, cli->nick);
		return;
	}

	if (argc < 2) {
		send_message(fd, -1, "%d %s :No recipient given (PRIVMSG)", ERR_NORECIPIENT, cli->nick);
		return;
	}

	while (*bufp != '\0') {
		if (*bufp == ',') {
			n++;
			*bufp = '\0';
		}
		
		bufp++;
	}

	bufp = args[1];

	for (i = 0; i < n; i++) {
		if ((target_chan = get_channel(bufp)) == NULL) {
			if ((target_cli = get_client_nick(bufp)) == NULL) {
				send_message(fd, -1, "%d %s %s :No such nick/channel", ERR_NOSUCHNICK, cli->nick, bufp);
				continue;
			}

			is_channel = 0;
		}

		if (is_channel && !in_channel(target_chan, cli->fd)) {
			send_message(fd, -1, "%d %s %s :Cannot send to channel", ERR_CANNOTSENDTOCHAN, cli->nick, bufp);
			continue;
		}

		is_channel ? send_to_channel(target_chan, cli->fd, "PRIVMSG %s %s", bufp, args[2]) : send_message(target_cli->fd, cli->fd, "PRIVMSG %s %s", bufp, args[2]);
	}
}

void user_quit(int cli_fd, char *quit_message)
{
	struct client *cli = get_client(cli_fd);
	send_to_all_visible(cli, "QUIT %s", quit_message);
	remove_client(cli_fd);
}

void initialize_global()
{
	register_command("PING", handle_ping);
	register_command("PRIVMSG", handle_privmsg);
}


#ifndef CHANNEL_STAT_H
#define CHANNEL_STAT_H

#include "channel.h"

void send_channel_greeting(struct channel *chan, struct client *cli);

void set_topic(struct channel *chan, char *topic);

void send_topic(struct channel *chan, struct client *cli);
void send_users(struct channel *chan, struct client *cli);

#endif

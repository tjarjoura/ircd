#ifndef CHANNEL_STAT_H
#define CHANNEL_STAT_H

#include "channel.h"

void send_channel_greeting(struct channel *chan, struct client *cli);
void initialize_channel_stat();
#endif

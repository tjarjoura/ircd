#ifndef STAT_H
#define STAT_H

#include "client.h"

extern const char *motd;
extern const char *server_version;
extern time_t server_start_time;

void initialize_stat();

void send_welcome(struct client *cli);

#endif

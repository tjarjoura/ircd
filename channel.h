#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_CHANNELS 40
#define MAX_JOIN 20

void initialize_channels();

int join_channel(char *chan_name, int cli_fd);
int part_channel(char *chan_name, int cli_fd);
int kick_user(char *chan_name, int cli_fd); 
int set_topic(char* chan_name, int cli_fd);

#endif

#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_JOIN 20

struct channel {
	char name[20];
	char topic[100];
	struct client *joined_users[MAX_JOIN];


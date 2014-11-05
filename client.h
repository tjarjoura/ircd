#ifndef CLIENT_H
#define CLIENT_H

#define MAX_CLIENTS 100

int new_client(int conn_fd);
int remove_client(int conn_fd);
int set_pass(int conn_fd, char *pass);
int set_nick(int conn_fd, char *nick);
int set_user(int conn_fd, char *username, int mode, char *realname);
struct client *get_client(int conn_fd);

#endif

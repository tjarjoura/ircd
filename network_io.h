#ifndef NETWORK_IO
#define NETWORK_IO

#include <unistd.h>

ssize_t readn(int fd, void *buf, size_t n_bytes);
ssize_t writen(int fd, void *buf, size_t n_bytes);

int ec_read(int fd, void *buf, size_t n_bytes);
void ec_write(int fd, void *buf, size_t n_bytes);

void send_message(int conn_fd, int sender_fd, char *msg, ...);

#endif

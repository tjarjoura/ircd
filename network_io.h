#ifndef NETWORK_IO
#define NETWORK_IO

#include <unistd.h>

ssize_t readn(int fd, void* buf, size_t n_bytes);
ssize_t writen(int fd, const void* buf, size_t n_bytes);

int ec_read(int fd, void* buf, size_t n_bytes);
void ec_write(int fd, const void* buf, size_t n_bytes);

void send_message(int sender_fd, int reply_numeric, char *msg, ...);

#endif

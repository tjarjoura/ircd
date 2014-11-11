#ifndef COMMAND_H
#define COMMAND_H

void register_command(char *cmd_name, void (*callback)(int fd, int argc, char **args));
void handle_command(int fd, int argc, char **args);

#endif

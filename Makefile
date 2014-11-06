CFLAGS = -Wall -g

all: channel.o client.o command.o network_io.o server.o
	gcc $(CFLAGS) -o ircd channel.o client.o command.o network_io.o server.o

channel.o: channel.c
	gcc $(CFLAGS) -c channel.c

client.o: client.c
	gcc $(CFLAGS) -c client.c

command.o: command.c
	gcc $(CFLAGS) -c command.c

network_io.o: network_io.c
	gcc $(CFLAGS) -c network_io.c

server.o: server.c
	gcc $(CFLAGS) -c server.c

clean:
	rm -f *.o ircd


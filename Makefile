CFLAGS = -Wall -g

all: channel.o channel_stat.o client.o command.o global.o global_stat.o main.o network_io.o
	gcc $(CFLAGS) -o ircd channel.o channel_stat.o client.o command.o global.o global_stat.o main.o network_io.o 

channel.o: channel.c
	gcc $(CFLAGS) -c channel.c

channel_stat.o: channel_stat.c
	gcc $(CFLAGS) -c channel_stat.c

client.o: client.c
	gcc $(CFLAGS) -c client.c

command.o: command.c
	gcc $(CFLAGS) -c command.c

global.o: global.c
	gcc $(CFLAGS) -c global.c	

global_stat.o: global_stat.c
	gcc $(CFLAGS) -c global_stat.c

main.o: main.c
	gcc $(CFLAGS) -c main.c

network_io.o: network_io.c
	gcc $(CFLAGS) -c network_io.c

stat.o: stat.c
	gcc $(CFLAGS) -c stat.c

clean:
	rm -f *.o ircd


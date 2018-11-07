CC=g++
CFLAGS=-std=c++14

all: cli-server cli-client

gpio.o: gpio.cpp
	$(CC) $(CFLAGS) -c gpio.cpp

server: gpio-server.cpp
	$(CC) $(CFLAGS) -c gpio-server gpio-server.cpp

client: gpio-client.cpp
	$(CC) $(CFLAGS) -c gpio-client gpio-client.cpp


cli-client: gpio-client.o gpio.o cli-client.cpp
	$(CC) $(CFLAGS) -o cli-client cli-client.cpp gpio-client.o gpio.o

cli-server: gpio-server.o gpio.o cli-server.cpp
	$(CC) $(CFLAGS) -o cli-server cli-server.cpp gpio-server.o gpio.o

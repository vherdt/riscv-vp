CC=g++
CFLAGS=-std=c++14

all: server client

gpio.o: gpio.cpp
	$(CC) $(CFLAGS) -c gpio.cpp

server: gpio.o gpio-server.cpp
	$(CC) $(CFLAGS) -o gpio-server gpio-server.cpp gpio.o
	
client: gpio.o gpio-client.cpp
	$(CC) $(CFLAGS) -o gpio-client gpio-client.cpp gpio.o
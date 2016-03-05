CC = c++
CFLAGS  = -std=c++11 -pthread -g -Wall -Wno-reorder

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp

client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp

clean: 
	$(RM) server client *.o *~

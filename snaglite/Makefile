CC = c++
CFLAGS  = -std=c++11 -pthread -g -Wall -Wno-reorder

all: snag snagServer

snag: snag.cpp
	$(CC) $(CFLAGS) -o snag snag.cpp

snagServer: snagServer.cpp
	$(CC) $(CFLAGS) -o snagServer snagServer.cpp

clean: 
	$(RM) snag snagServer *.o *~

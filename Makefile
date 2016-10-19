#Makefile for tcp-client and tcp-server
client = tcp-client.o
server = tcp-server.o

CC = g++
CFLAGS = -g -Wall -std=c++11

all: tcp-client tcp-server

tcp-server: $(server)
	$(CC) $(CFLAGS) -o tcp-server $(server)
	
tcp-server.o : tcp-server.cpp tcp.h
	$(CC) $(CFLAGS) -c tcp-server.cpp

tcp-client : $(client)
	$(CC) $(CFLAGS) -o tcp-client $(client)

tcp-client.o : tcp-client.cpp tcp.h
	$(CC) $(CFLAGS) -c tcp-client.cpp

.PHONY :

clean :
	-rm tcp-client tcp-server *.o




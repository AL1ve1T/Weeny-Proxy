# g++ -c --std=c++11

CC=g++ -g
CXXFLAGS= -c -w -m64 --std=c++0x -Wall

all: weeny_proxy

weeny_proxy: main.o server.o proxy.o
	$(CC) --std=c++0x -pthread -o weeny_proxy main.o server.o proxy.o includes.hpp

main.o: main.cpp
	$(CC) $(CXXFLAGS) main.cpp

server.o: server.cpp
	$(CC) $(CXXFLAGS) server.cpp

proxy.o: proxy.cpp
	$(CC) $(CXXFLAGS) proxy.cpp

clean:
	rm -rf *.o

all:server client

server: server.cc message.h
	g++ -std=c++0x server.cc -g -o server

client: client.cc message.h
	g++ -std=c++0x client.cc -g -o client

clean: 
	rm -rf server client

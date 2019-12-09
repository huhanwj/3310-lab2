all:server client test

server: server.cc message.h
	g++ server.cc -g -o server

client: client.cc message.h
	g++ client.cc -g -o client

clean: 
	rm -rf server client

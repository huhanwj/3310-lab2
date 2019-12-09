all:server client test

server: server.cc message.h
	g++ server.cc -g -o server

client: client.cc message.h
	g++ client.cc -g -o client

test: test_cin.cc
	g++ test_cin.cc -g -o test

clean: 
	rm -rf server client

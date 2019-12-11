all:server client server1 client1

server1: server1.cc message.h
	g++ -std=c++0x server1.cc -g -o server1

client1: client1.cc message.h
	g++ -std=c++0x client1.cc -g -o client1
	
server: server.cc message.h
	g++ -std=c++0x server.cc -g -o server

client: client.cc message.h
	g++ -std=c++0x client.cc -g -o client

clean: 
	rm -rf server client

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sstream>
#include <string>
#include <netdb.h>
#include <algorithm>

#include "message.h"
#include "client.h"
#define BUFLEN 256
using namespace std;

int main(int argc, char *argv[])
{
	unsigned short udp_port = 0;
	const char* server_host = "127.0.0.1";
	//process input arguments
	if ((argc != 3) && (argc != 5))
	{
		cout << "Usage: " << argv[0];
		cout << " [-s <server_host>] -p <udp_port>" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
			else if (strcmp(argv[i], "-s") == 0)
			{
				server_host = argv[++i];
				if (argc == 3)
				{
					cout << "Usage: " << argv[0];
					cout << " [-s <server_host>] -p <udp_port>" << endl;
					return 1;
				}
			}
			else
			{
				cout << "Usage: " << argv[0];
				cout << " [-s <server_host>] -p <udp_port>" << endl;
				return 1;
			}
		}
	}
	int sock; // socket descriptor
	sockaddr_in remote; // socket address for remote side
	char buf[BUFLEN]; // buffer for response from remote
	hostent *hp; // address of remote host
	int msglen; // actual length of the message
	// create the socket
	sock=socket(AF_INET, SOCK_DGRAM, 0);
	// error handling
	if(sock<0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	// designate the addressing family
	remote.sin_family=AF_INET;
	// get the address from the remote host and store
	hp=gethostbyname(server_host);
	memcpy(&remote.sin_addr, hp->h_addr, hp->h_length);
	remote.sin_port=udp_port;
	Client_State_T client_state = WAITING;
	string in_cmd;
	Cmd_Msg_T client;
	Cmd_Msg_T remake;
	while(true)
	{
		usleep(100);
		switch(client_state)
		{
			case WAITING:
			{
				cout<<"$ ";
				cin>>in_cmd;
				
				if(in_cmd == "ls")
				{
					client.cmd=CMD_LS;
					const char* convert_cmd=to_string(CMD_LS).c_str();
					// send the command to the remote side
					sendto(sock,convert_cmd,strlen(convert_cmd),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state = PROCESS_LS;
				}
				else if(in_cmd == "send")
				{
					client.cmd=CMD_SEND;
					const char* convert_cmd=to_string(CMD_SEND).c_str();
					// send the command to the remote side
					sendto(sock,convert_cmd,strlen(convert_cmd),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state = PROCESS_SEND;
				}
				else if(in_cmd == "remove")
				{
					client.cmd=CMD_REMOVE;
					const char* convert_cmd=to_string(client.cmd).c_str();
					// send the command to the remote side
					sendto(sock,convert_cmd,strlen(convert_cmd),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state = PROCESS_REMOVE;
				}
				else if(in_cmd == "shutdown")
				{
					client.cmd=CMD_SHUTDOWN;
					const char* convert_cmd=to_string(client.cmd).c_str();
					// send the command to the remote side
					sendto(sock,convert_cmd,strlen(convert_cmd),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state = SHUTDOWN;
				}
				else if(in_cmd == "quit")
				{
					client_state = QUIT;
				}
				else
				{
					cout<<" - wrong command."<<endl;
					client_state = WAITING;
				}
				break;
			}
			case PROCESS_LS:
			{  
				msglen=read(sock,buf,BUFLEN); //read the msg from server
				buf[msglen]='\0';
				int current=atoi(buf);
				if(current!=CMD_LS){ //bad first cmd back, not ls
					cout << " - error or incorrect response from server.\n";
					client.error=1;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state=WAITING;
					break;
				}
				else{
					client.error=0;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
				}
				msglen=read(sock,buf,BUFLEN); //read the msg from server
				buf[msglen]='\0';
				client.size=atoi(buf); // convert the received string to int, received msg from server, size of the file list
				if(!client.size){ //size=0
					cout << " - Server backup folder is empty.\n";
				}
				else{ // read the filename and print it out
					for(int i=0;;i++){
						if(i<client.size){
							msglen=read(sock,buf,BUFLEN);
							buf[msglen]='\0';
							cout << " - filename: " << buf << "\n";
						}
						else
							break;
					}
				}
				client=remake;
				client_state = WAITING;
				break;
			}
			case PROCESS_SEND:
			{
				const char* error_send;
				const char* filesize_send;
				const char* filename_send;
				const char* port_send;
				int overwrite=0;
				cin >> client.filename;
				FILE* upload_file=fopen(client.filename,"r+");
				if(upload_file){
					fseek(upload_file,0,SEEK_END);// using fseek to go through the file
					client.size=ftell(upload_file); // using ftell to tell the size of file
					cout<< " - filesize:"<<client.size<<"\n";
					client.error=0;
					error_send=to_string(client.error).c_str(); //send the error
					sendto(sock,error_send,strlen(error_send),0,(struct sockaddr*)&remote,sizeof(remote));
					filename_send=client.filename; //send the filename
					sendto(sock,filename_send,strlen(filename_send),0,(struct sockaddr*)&remote,sizeof(remote));
					filesize_send=to_string(client.size).c_str(); //send the filesize
					sendto(sock,filesize_send,strlen(filesize_send),0,(struct sockaddr*)&remote,sizeof(remote));
					fseek(upload_file,0,SEEK_SET); //reset the file pointer to the starting point
					msglen=read(sock,buf,BUFLEN); //error checking, bad first cmd back(not being send)
					buf[msglen]='\0';
					int current=atoi(buf);
					if(current!=CMD_SEND){
						cout<<"send error 1\n";
						cout << " - error or incorrect response from server.\n";
						client.error=1;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					else{
						client.error=0;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					}
					msglen=read(sock,buf,BUFLEN);//read the back msg from server to check duplicate
					buf[msglen]='\0';
					client.error=atoi(buf);
					if(client.error==2){
						string choice;
						cout<<" - file exists. overwrite? (y/n):";
						cin >> choice;
						if(choice=="y")
							client.error=0;
						else
							client.error=2;
						const char* overwrite_co=to_string(client.cmd).c_str();// send the cmd before the overwrite decision
						sendto(sock,overwrite_co,strlen(overwrite_co),0,(struct sockaddr*)&remote,sizeof(remote));
						msglen=read(sock,buf,BUFLEN);//check if error=1, cmd back is not send
						buf[msglen]='\0';
						if(atoi(buf)==1){
							cout<<"send error 2\n";
							cout << " - error or incorrect response from server.\n";
							fclose(upload_file);
							client_state=WAITING;
							break;
						}
						const char* overwrite_de=to_string(client.error).c_str();// send the user's overwrite decision to server
						sendto(sock,overwrite_de,strlen(overwrite_de),0,(struct sockaddr*)&remote,sizeof(remote));
						if(client.error==2){//not to overwrite
							fclose(upload_file);
							client_state=WAITING;
							break;
						}
					}
					msglen=read(sock,buf,BUFLEN);//check if back cmd is wrong
					buf[msglen]='\0';
					if(atoi(buf)!=CMD_SEND){
						cout<<buf<<"\n";
						cout<<"send error 3\n";
						cout << " - error or incorrect response from server.\n";
						client.error=1;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					else{
						client.error=0;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					}
					msglen=read(sock,buf,BUFLEN);//check if server back error msg=0
					buf[msglen]='\0';
					if(atoi(buf)==1){
						cout<<"send error 4\n";
						cout << " - error or incorrect response from server.\n";
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					msglen=read(sock,buf,BUFLEN);//check if server file opening failed
					buf[msglen]='\0';
					if(atoi(buf)==1){
						cout<<"send error 4\n";
						cout << " - error or incorrect response from server.\n";
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					msglen=read(sock,buf,BUFLEN);//get the allocated port number from the server
					buf[msglen]='\0';
					client.port=atoi(buf);
					cout<<"received port: "<<buf<<"\n";
					cout <<" - TCP port: "<<client.port<<"\n";
					int tcp; //socket descriptor
					sockaddr_in tcp_remote; //socket address for remote side
					char tcp_buf[DATA_BUF_LEN];//buffer for response from remote
					hostent *tp; //address for remote host;
					int tcp_msglen;//length of the message
					tcp=socket(AF_INET,SOCK_STREAM,0);//create the socket
					tcp_remote.sin_family=AF_INET;//designate the addressing family
					tp=gethostbyname(server_host);//get the address 
					memcpy(&tcp_remote.sin_addr,tp->h_addr,tp->h_length) ;//store the address
					tcp_remote.sin_port=client.port;
					msglen=read(sock,buf,BUFLEN);//check if server socket accept failed
					buf[msglen]='\0';
					if(atoi(buf)==1){
						cout<<"send error 5\n";
						cout << " - error or incorrect response from server.\n";
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					//connect to server 
					if(connect(tcp,(struct sockaddr*)&tcp_remote,sizeof(tcp_remote))<0){//connection failed
						cout << " - connection error!\n";
						close(tcp);
						client.error=1;
						const char* error_terminate=to_string(client.error).c_str();//send error
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
						fclose(upload_file);
						client_state=WAITING;
						break;
					}
					bool keepread=true;
					//read and send the file
					while(keepread){
						tcp_msglen=fread(tcp_buf,sizeof(char),DATA_BUF_LEN,upload_file);
						if(tcp_msglen<DATA_BUF_LEN){
							tcp_buf[tcp_msglen]='\0';
							keepread=false;
						}
						write(tcp,tcp_buf,tcp_msglen);
					}
					fclose(upload_file);
					close(tcp);
					// check for ACK msg
					msglen=read(sock,buf,BUFLEN);
					buf[msglen]='\0';
					//back cmd msg is not ACK
					if(atoi(buf)!=CMD_ACK){
						cout << " - error or incorrect response from server.\n";
						client.error=1;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
						client_state=WAITING;
						break;
					}
					else{
						client.error=0;
						const char* error_terminate=to_string(client.error).c_str();
						sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					}
					// check for back error msg
					msglen=read(sock,buf,BUFLEN);
					buf[msglen]='\0';
					if(atoi(buf)==0)
						cout<< " - file transmission is completed.\n";
					else
						cout<<" - file transmission is failed.\n";
				}
				else{
					cout<<" - cannot open file: "<<client.filename<< "\n";
					client.error=1;
					error_send=to_string(client.error).c_str();
					sendto(sock,error_send,strlen(error_send),0,(struct sockaddr*)&remote,sizeof(remote));
				}
				client=remake;
				client_state = WAITING;
				break;
			}
			case PROCESS_REMOVE:
			{	           
				const char* filename_send;
				cin>>client.filename;
				filename_send=client.filename;
				sendto(sock,filename_send,strlen(filename_send),0,(struct sockaddr*)&remote,sizeof(remote));
				//read the back ACK
				msglen=read(sock,buf,BUFLEN);
				buf[msglen]='\0';
				if(atoi(buf)!=CMD_ACK){
					cout << " - error or incorrect response from server.\n";
					client.error=1;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state=WAITING;
					break;
				}
				else{
					client.error=0;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
				}
				//after receiving ACK, read the error message
				msglen=read(sock,buf,BUFLEN);
				buf[msglen]='\0';
				client.error=atoi(buf);
				if(client.error>1){// error=3
					cout<<" - server error. remove failed.\n";
				}
				else if(client.error==1){//error=1
					cout<<" - file doesn't exist.\n";
				}
				else{//error=0
					cout<<" - file is removed.\n";
				}
				client_state = WAITING;
				break;
			}	
			case SHUTDOWN:
			{	            
				//read the back ACK
				msglen=read(sock,buf,BUFLEN);
				buf[msglen]='\0';
				if(atoi(buf)!=CMD_ACK){
					cout << " - error or incorrect response from server.\n";
					client.error=1;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					client_state=WAITING;
					break;
				}
				else{
					client.error=0;
					const char* error_terminate=to_string(client.error).c_str();
					sendto(sock,error_terminate,strlen(error_terminate),0,(struct sockaddr*)&remote,sizeof(remote));
					cout<<" - server is shutdown.\n";
				}
				client_state=WAITING;
				break;	            
			}
			case QUIT:
			{	         
				close(sock);
				exit(0);
			}
			default:
			{
				client_state = WAITING;
				break;
			}    
		}
	}

	return 0;
}

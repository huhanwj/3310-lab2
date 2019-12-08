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
	sockaddr_in local; //socket address for local side
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
	hp=gethostbyname(argv[1]);
	memcpy(&remote.sin_family, hp->h_addr, hp->h_length);
	remote.sin_port=atoi(argv[2]);

	Client_State_T client_state = WAITING;
	string in_cmd;
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
					const char* convert_cmd=to_string(CMD_LS).c_str();
	            	// send the command to the remote side
					sendto(sock, convert_cmd, strlen(convert_cmd),0,(struct sockaddr*)&remote, sizeof(remote));
	                client_state = PROCESS_LS;
	            }
	            else if(in_cmd == "send")
	            {
	                client_state = PROCESS_SEND;
	            }
	            else if(in_cmd == "remove")
	            {
	                client_state = PROCESS_REMOVE;
	            }
	            else if(in_cmd == "shutdown")
	            {
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
				int size=0; // received msg from server, size of the file list
				msglen=read(sock,buf,BUFLEN); //read the msg from server
				buf[msglen]='\0';
				size=atoi(buf); // convert the received string to int
				if(!size){ //size=0
					cout << " - server backup folder is empty.";
				}
				else{ // read the filename and print it out
					for(int i=0;i<size;i++){
						msglen=read(sock,buf,BUFLEN);
						buf[msglen]='\0';
						cout << " - filename: " << buf << "\n";
					}
				}
		        client_state = WAITING;
	            break;
	        }
	        case PROCESS_SEND:
	        {
	            
		        client_state = WAITING;
		        break;
	        }
	        case PROCESS_REMOVE:
	        {	           
	            client_state = WAITING;
	            break;
	        }	
	        case SHUTDOWN:
	        {	            
	            break;	            
	        }
	        case QUIT:
	        {	         
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

#include <sys/types.h>
#include <sys/stat.h>
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

#include "message.h"
#include "server.h"

#define Dir "backup"

using namespace std;

Server_State_T server_state;
string cmd_string[] = {" ", "CMD_LS", "CMD_SEND","CMD_GET","CMD_REMOVE", "CMD_SHUTDOWN"};

int main(int argc, char *argv[])
{
	unsigned short udp_port = 0;
	if ((argc != 1) && (argc != 3))
	{
		cout << "Usage: " << argv[0];
		cout << " [-p <udp_port>]" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		//process input arguments
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
			else
			{
				cout << "Usage: " << argv[0];
				cout << " [-p <udp_port>]" << endl;
				return 1;
			}
		}
	}
	int sk; // socket descriptor
	sockaddr_in remote; // socket address for remote
	sockaddr_in local; // socket address for us
	char buf[256]; //buffer from remote
	char retbuf[256]; //buffer to remote
	socklen_t rlen = sizeof(remote) ; // length of remote address
	socklen_t local_len = sizeof(local) ; // length of local address
	int mesglen ; // actual length of message
	// create the socket
	sk = socket(AF_INET,SOCK_DGRAM,0) ;
	// set up the socket
	local.sin_family = AF_INET ; // internet familyx
	local.sin_addr.s_addr = INADDR_ANY ; // wild card machine address
	local.sin_port = udp_port ;
	// bind the name (address) to a port
	bind(sk,(struct sockaddr *)&local,sizeof(local)) ;
	// get the port name and print it out
	getsockname(sk,(struct sockaddr *)&local, &local_len) ;
	cout << "socket has port " << local.sin_port << "\n" ;
	// cout << "socket has addr " << local.sin_addr .s_addr << "\n" ;
	int cmd_i=0; // init no command received
	int check=checkDirectory(Dir);
	vector<string> backup_file;
	while(true)
	{
		usleep(100);
		switch(server_state)
		{
			case WAITING:
			{
				cout << "Waiting UDP Command @: " << udp_port;
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
				buf[mesglen]='\0';
				cmd_i=atoi(buf);
				if(cmd_i)
					cout << "[CMD RECEIVED]: "<< cmd_string[cmd_i] << "\n";
				switch (cmd_i)
				{
					case 1:
					{
						server_state = PROCESS_LS;
						break;
					}
                    case 2:
                    {
                        server_state=PROCESS_SEND;
                        break;
                    }
                    case 4:
                    {
                        server_state=PROCESS_REMOVE;
                        break;
                    }
                    case 5:
                    {
                        server_state=SHUTDOWN;
                        break;
                    }
                    default:
                    {
                        server_state=WAITING;
                        break;
                    }
				}
				break;
			}
			case PROCESS_LS:
			{
				cout << " - ";
				int get=getDirectory(Dir,backup_file);
				int size=backup_file.size;
				const char* msgsend=to_string(size).c_str();
				if(!size)
					cout << "server backup folder is empty.";
				else
				server_state = WAITING;
				break;
			}
			case PROCESS_SEND:
			{
				server_state = WAITING;
				break;
			}
			case PROCESS_REMOVE:
			{
				server_state = WAITING;
				break;
			}
			case SHUTDOWN:
			{
			}
			default:
			{
				server_state = WAITING;
				break;
			}
		}
	}
	return 0;
}

//this function check if the backup folder exist
int checkDirectory (string dir)
{
	DIR *dp;
	if((dp  = opendir(dir.c_str())) == NULL) {
		//cout << " - error(" << errno << ") opening " << dir << endl;
		if(mkdir(dir.c_str(), S_IRWXU) == 0)
			cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
		else
			cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
		return errno;
	}
	closedir(dp);
}


//this function is used to get all the filenames from the
//backup directory
int getDirectory (string dir, vector<string> &files)
{
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(dir.c_str())) == NULL) {
		//cout << " - error(" << errno << ") opening " << dir << endl;
		if(mkdir(dir.c_str(), S_IRWXU) == 0)
			cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
		else
			cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
		return errno;
	}

	int j=0;
	while ((dirp = readdir(dp)) != NULL) {
		//do not list the file "." and ".."
		if((string(dirp->d_name)!=".") && (string(dirp->d_name)!=".."))
			files.push_back(string(dirp->d_name));
	}
	closedir(dp);
	return 0;
}
//this function check if the file exists
bool checkFile(const char *fileName)
{
	ifstream infile(fileName);
	return infile.good();
}


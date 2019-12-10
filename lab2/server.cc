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
	// cout << "socket has addr " << local.sin_addr .s_addr << "\n" ;
	int check=checkDirectory(Dir); //create the backup directory
	Cmd_Msg_T server;
	Cmd_Msg_T remake;
	remake.cmd=0;
	remake.error=0;
	remake.port=0;
	remake.size=0;
	strcpy(remake.filename,"");
	while(true)
	{
		usleep(100);
		switch(server_state)
		{
			case WAITING:
			{
				server.cmd=0;
				cout << "Waiting UDP Command @: " << local.sin_port<<"\n";
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
				buf[mesglen]='\0';
				server.cmd=atoi(buf);
				if(server.cmd)
					cout << "[CMD RECEIVED]: "<< cmd_string[server.cmd] << "\n";
				switch (server.cmd)
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
				vector<string> backup_file; //list of the backup files
				const char* cmd_send=to_string(server.cmd).c_str();
				sendto(sk,cmd_send,strlen(cmd_send),0,(struct sockaddr*)&remote,sizeof(remote));// back cmd msg
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// error checking
				buf[mesglen]='\0';
				if(atoi(buf)==1){
					cout << " - error or incorrect command from client.\n";
					server_state=WAITING;
					break;
				}
				const char* filename_send;
				int get=getDirectory(Dir,backup_file);
				server.size=backup_file.size();
				const char* size_send=to_string(server.size).c_str();
				sendto(sk,size_send,strlen(size_send),0,(struct sockaddr*)&remote,sizeof(remote));// back size msg
				if(!server.size)
					cout << " - Server backup folder is empty.\n";
				else{
					for(int i=0;i<server.size;i++){
						cout << " - filename: " << backup_file[i] << "\n";
						filename_send=backup_file[i].c_str();
						sendto(sk,filename_send,strlen(filename_send),0,(struct sockaddr*)&remote,sizeof(remote));
					}
				}
				server=remake;
				server_state = WAITING;
				break;
			}
			case PROCESS_SEND:
			{
				int overwrite=0;
				bool exist=false;
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// first receive the error msg
				buf[mesglen]='\0';
				server.error=atoi(buf);
				if(server.error==0){
					mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// then receive the filename
					buf[mesglen]='\0';
					strcpy(server.filename,buf);
					cout << " - filename: "<<server.filename<<"\n";
					mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// then receive the filesize
					buf[mesglen]='\0';
					server.size=atoi(buf);
					cout<<" - filesize: "<<server.size<<"\n";
					exist=checkFile((string("./backup/")+string(server.filename)).c_str());
					const char* error_back; // existance checking msg
					const char* cmd_back; // existance checking condition
					if(exist){
						cout <<" - file ./backup/"<<server.filename<< " exist; overwrite?\n";
						cmd_back=to_string(server.cmd).c_str();// sending back the condition
						sendto(sk,cmd_back,strlen(cmd_back),0,(struct sockaddr*)&remote,sizeof(remote));
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// error checking
						buf[mesglen]='\0';
						if(atoi(buf)==1){
							cout << " - error or incorrect command from client.\n";
							server_state=WAITING;
							break;
						}
						server.error=2;
						error_back=to_string(server.error).c_str();//send back the error=2
						sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// error checking
						buf[mesglen]='\0';
						if(atoi(buf)!=CMD_SEND){
							cout << " - error or incorrect command from client.\n";
							server.error=1;
							error_back=to_string(server.error).c_str();
							sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=1
							server_state=WAITING;
							break;
						}
						else{
							server.error=0;
							error_back=to_string(server.error).c_str();
							sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=0
						}
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// receive the user's decision
						buf[mesglen]='\0';
						server.error=atoi(buf);
						if(server.error==2){//not to overwrite
							cout<<" - do not overwrite.\n";
							server_state=WAITING;
							break;
						}
						else
							cout<<" - overwrite the file.\n";
					}
					else{
						server.cmd=CMD_SEND;
						cmd_back=to_string(server.cmd).c_str();// sending back the condition
						sendto(sk,cmd_back,strlen(cmd_back),0,(struct sockaddr*)&remote,sizeof(remote));
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// error checking
						buf[mesglen]='\0';
						if(atoi(buf)==1){
							cout << " - error or incorrect command from client.\n";
							server_state=WAITING;
							break;
						}
						error_back=to_string(server.error).c_str();// send back the error=0
						sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));
					}
					cmd_back=to_string(server.cmd).c_str();// sending back the condition
					sendto(sk,cmd_back,strlen(cmd_back),0,(struct sockaddr*)&remote,sizeof(remote));
					mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);// error checking
					buf[mesglen]='\0';
					if(atoi(buf)==1){
						cout << " - error or incorrect command from client.\n";
						server_state=WAITING;
						break;
					}
					error_back=to_string(server.error).c_str();// send back the error=0
					sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));
					// open a new file in backup dir
					FILE* backup=fopen((string("./backup/")+string(server.filename)).c_str(),"w+");
					if(!backup){
						cout <<"open file "<<server.filename<<" error.\n";
						server.error=1;
						error_back=to_string(server.error).c_str();
						sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=1
						server_state=WAITING;
						break;
					}
					else{
						server.error=0;
						error_back=to_string(server.error).c_str();
						sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=0
						int tcp,tcp2; // socket descriptors
						sockaddr_in tcp_remote; // socket address for remote
						sockaddr_in tcp_local; // socket address for us
						char tcp_buf[DATA_BUF_LEN]; // buffer from remote
						char tcp_retbuf[DATA_BUF_LEN]; //buffer to remote
						int tcp_rlen = sizeof(tcp_remote) ; // length of remote address
						socklen_t tcp_len = sizeof(tcp_local) ; // length of local address
						int moredata = 1 ; // keep processing or quit
						int tcp_mesglen ; // actual length of message
						tcp=socket(AF_INET,SOCK_STREAM,0);// create the socket
						tcp_local.sin_family = AF_INET ; // internet family
						tcp_local.sin_addr.s_addr = INADDR_ANY ; // wild card machine address
						tcp_local.sin_port = 0 ; // let system choose the port
						// bind the name (address) to a port
						bind(tcp,(struct sockaddr *)&tcp_local,sizeof(tcp_local)) ;
						// get the port name and print it out
						getsockname(tcp,(struct sockaddr *)&tcp_local,&tcp_len) ;
						server.port=tcp_local.sin_port;
						const char* port_back=to_string(server.port).c_str();
						sendto(sk,port_back,strlen(port_back),0,(struct sockaddr*)&remote,sizeof(remote));
						listen(tcp, 1);
						cout <<" - listen @: " << server.port <<"\n";
						// wait for connection request, then close old socket
						tcp2 = accept(tcp, (struct sockaddr *)0, (socklen_t*)0) ;
						close(tcp);
						cout<<tcp2<<'\n';
						//accept failed
						if(tcp2==-1){ 
							cout << " - server connection failed!\n";
							server.error=1;
							fclose(backup);
							error_back=to_string(server.error).c_str();
							sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=1
							server_state=WAITING;
							break;
						}
						//accept succeeded
						else{
							server.error=0;
							error_back=to_string(server.error).c_str();
							sendto(sk,error_back,strlen(error_back),0,(struct sockaddr*)&remote,sizeof(remote));// send error=0
						}
						// error checking if client tcp connection failed
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
						buf[mesglen]='\0';
						if(atoi(buf)==1){
							cout << " - client connection failed!\n";
							fclose(backup);
							close(tcp2);
							server_state=WAITING;
							break;
						}
						else{
							cout<<" - connected with client.\n";
						}
						// receive and write the file
						bool keepwrite=true;
						while(keepwrite){
							tcp_mesglen=read(tcp2,tcp_buf,DATA_BUF_LEN);
							cout << " - total bytes received: "<<tcp_mesglen<<"\n";
							//if any message reception error happened, break the loop
							if(tcp_mesglen<0){
								cout << " - message reception error.\n";
								break;
							}
							if(tcp_mesglen<DATA_BUF_LEN){
								tcp_buf[tcp_mesglen]='\0';
								keepwrite=false;
							}
							fwrite(tcp_buf,sizeof(char),tcp_mesglen,backup);
						}
						fclose(backup);
						close(tcp2);
						const char* ACK_back;
						const char* ACK_error;
						server.cmd=CMD_ACK;
						ACK_back=to_string(server.cmd).c_str();
						sendto(sk,ACK_back,strlen(ACK_back),0,(struct sockaddr*)&remote,sizeof(remote));
						// error checking if client not receiving ACK
						mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
						if(atoi(buf)==1){
							cout <<" - connection failed!\n";
							server_state=WAITING;
							break;
						}
						// send the error message to the client
						if(!keepwrite){
							cout << " - "<<server.filename<<" has been received.\n";
							server.error=0;
							ACK_error=to_string(server.error).c_str();
							sendto(sk,ACK_error,strlen(ACK_error),0,(struct sockaddr*)&remote,sizeof(remote));
							cout<<"send acknowledgement.\n";	
						}
						else{
							server.error=1;
							ACK_error=to_string(server.error).c_str();
							sendto(sk,ACK_error,strlen(ACK_error),0,(struct sockaddr*)&remote,sizeof(remote));
							cout<<"send acknowledgement.\n";	
						}
					}
				}
				else{// file open error at client side
					cout << " - file open failure at client side.\n";
					server_state=WAITING;
					break;
				}
				server_state = WAITING;
				break;
			}
			case PROCESS_REMOVE:
			{
				const char* ACK_back;
				const char* ACK_error;
				string name;
				//read the filename from client
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
				buf[mesglen]='\0';
				name=string(buf);
				strcpy(server.filename,name.c_str());
				//check for existance
				bool exist=checkFile((string("./backup/")+string(server.filename)).c_str());
				if(exist){
					//remove result
					int result=remove((string("./backup/")+string(server.filename)).c_str());
					//send ACK back
					server.cmd=CMD_ACK;
					ACK_back=to_string(server.cmd).c_str();
					sendto(sk,ACK_back,strlen(ACK_back),0,(struct sockaddr*)&remote,sizeof(remote));
					// error checking if client not receiving ACK
					mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
					if(atoi(buf)==1){
						cout <<" - connection failed!\n";
						server_state=WAITING;
						break;
					}
					//remove success?
					if(result<0){
						server.error=3; //remove failed
						cout<<" - remove error.\n";
						// send the remove result to client
						ACK_error=to_string(server.error).c_str();
						sendto(sk,ACK_error,strlen(ACK_error),0,(struct sockaddr*)&remote,sizeof(remote));
						cout<<" - send acknowledgement.\n";
					}
					else{
						server.error=0;//remove succeed
						cout << " - ./backup/"<<server.filename<< " has been removed.\n";
						// send the remove result to client
						ACK_error=to_string(server.error).c_str();
						sendto(sk,ACK_error,strlen(ACK_error),0,(struct sockaddr*)&remote,sizeof(remote));
						cout<<" - send acknowledgement.\n";
					}
				}
				else{
					//send ACK back
					server.cmd=CMD_ACK;
					ACK_back=to_string(server.cmd).c_str();
					sendto(sk,ACK_back,strlen(ACK_back),0,(struct sockaddr*)&remote,sizeof(remote));
					// error checking if client not receiving ACK
					mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
					if(atoi(buf)==1){
						cout <<" - connection failed!\n";
						server_state=WAITING;
						break;
					}
					server.error=1;//file not exist
					cout << " - file doesn't exist.\n";
					// send the remove result to client
					ACK_error=to_string(server.error).c_str();
					sendto(sk,ACK_error,strlen(ACK_error),0,(struct sockaddr*)&remote,sizeof(remote));
					cout<<" - send acknowledgement.\n";
				}
				server_state = WAITING;
				break;
			}
			case SHUTDOWN:
			{
				const char* ACK_back;
				const char* ACK_error;
				server.cmd=CMD_ACK;
				//send ACK back
				ACK_back=to_string(server.cmd).c_str();
				sendto(sk,ACK_back,strlen(ACK_back),0,(struct sockaddr*)&remote,sizeof(remote));
				// error checking if client not receiving ACK
				mesglen=recvfrom(sk,buf,256,0,(struct sockaddr *)&remote, &rlen);
				if(atoi(buf)==1){
					cout <<" - connection failed!\n";
					server_state=WAITING;
					break;
				}
				else{
					cout<<" - send acknowledgement.\n";
					close(sk);
					exit(0);
				}

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


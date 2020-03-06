#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "potato.h"

using namespace std;

class Ringmaster{
private:
	int status;
  	int ringmaster_fd;
  	struct addrinfo host_info;
  	struct addrinfo *host_info_list;
  	const char * hostname;
  	const char * port;
public:
	int num_players;
	int num_hops;
	vector<int> player_connection_fd;
	vector<int> player_port;
	vector<string> player_IP;

	Ringmaster(char ** argv):status(0),ringmaster_fd(0),hostname(NULL){
		port = argv[1];
		num_players = atoi(argv[2]);
		num_hops = atoi(argv[3]);
		player_connection_fd.resize(num_players);
		player_port.resize(num_players);
		player_IP.resize(num_players);
	}

	void init_print(){
		printf("Potato Ringmaster\n");
		printf("Players = %d\n", num_players);
		printf("Hops = %d\n",num_hops);
	}

	void get_address_info(){
		memset(&host_info, 0, sizeof(host_info));
		host_info.ai_family   = AF_UNSPEC;
		host_info.ai_socktype = SOCK_STREAM;
		host_info.ai_flags    = AI_PASSIVE;

		status = getaddrinfo(hostname, port, &host_info, &host_info_list);
		if (status != 0) {
		  cerr << "Error: cannot get address info for host" << endl;
		  cerr << "  (" << hostname << "," << port << ")" << endl;
		  freeaddrinfo(host_info_list);
		  exit(EXIT_FAILURE);
	  	} 
	}

	void create_socket(){
		//socket
		ringmaster_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
		if (ringmaster_fd == -1) {
		  cerr << "Error: cannot create socket" << endl;
		  cerr << "  (" << hostname << "," << port << ")" << endl;
		  freeaddrinfo(host_info_list);
		  exit(EXIT_FAILURE);
		}
		//bind
		int yes = 1;
  		status = setsockopt(ringmaster_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  		status = bind(ringmaster_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  		if (status == -1) {
    		cerr << "Error: cannot bind socket" << endl;
    		cerr << "  (" << hostname << "," << port << ")" << endl;
    		freeaddrinfo(host_info_list);
    		exit(EXIT_FAILURE);
  		}
  		//ringmaster listen
  		status = listen(ringmaster_fd, 100);
  		if (status == -1) {
    		cerr << "Error: cannot listen on socket" << endl; 
    		cerr << "  (" << hostname << "," << port << ")" << endl;
    		freeaddrinfo(host_info_list);
    		exit(EXIT_FAILURE);
  		}

  		freeaddrinfo(host_info_list);
	}

	void establish_connections(){
		//accept num_players of connections
  		for(int i=0;i<num_players;i++){
  			struct sockaddr_storage socket_addr;
  			socklen_t socket_addr_len = sizeof(socket_addr);
  			player_connection_fd[i] = accept(ringmaster_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  			if (player_connection_fd.back() == -1) {
    			cerr << "Error: cannot accept connection on socket" << endl;
    			exit(EXIT_FAILURE);
			}
			//get player's IP
			struct sockaddr_in *temp = (struct sockaddr_in *)&socket_addr;
                        player_IP[i] = inet_ntoa(temp->sin_addr);
			//send message to players(player_ID),num_players
			send(player_connection_fd[i],&i,sizeof(i),0);
			send(player_connection_fd[i],&num_players,sizeof(num_players),0);
  			//receive player's port number
  			recv(player_connection_fd[i],&player_port[i],sizeof(player_port[i]),MSG_WAITALL);
  		} 
	}

	void send_circle_info(){
		int c = 0 ;//the indicator of each player's connection with another
		for(int i=0;i<num_players;i++){
			int next = (i+1)%num_players;
			//send neighbours message to players
			int IP_SIZE = player_IP[next].length();
			send(player_connection_fd[i],&IP_SIZE,sizeof(IP_SIZE),0);
			send(player_connection_fd[i],player_IP[next].c_str(),IP_SIZE,0);
			send(player_connection_fd[i],&player_port[next],sizeof(player_port[next]),0);
			recv(player_connection_fd[i],&c,sizeof(c),MSG_WAITALL);
			printf("Player %d is ready to play\n",i);
		}
	}

	void play_game(){
		//create a brand new potato
		Potato potato;
		potato.hops = num_hops;
		//handle situation where hops is initialized to 0
		if(num_hops == 0){
			for(int i=0;i<num_players;i++){
				send(player_connection_fd[i],&potato,sizeof(potato),0);
			}
			//sleep(1);
			return;
		}
		//create a random number
		//srand((unsigned int)time(NULL));
		int random = rand() % num_players;
		//start the game! Passing the first potato
		printf("Ready to start the game, sending potato to player %d\n", random);
		if(send(player_connection_fd[random],&potato,sizeof(potato),0)!=sizeof(potato)){
			perror("send a broken potato!\n");
		}
		//normal situation(select)
		fd_set readfds;
		FD_ZERO(&readfds);
  		for(int i=0;i<num_players;i++){
  			FD_SET(player_connection_fd[i], &readfds);
  		}
  		int fdmax = player_connection_fd[num_players-1]+1;
 		int temp=select(fdmax,&readfds,NULL,NULL,NULL);
 		if(temp == -1){
 			cerr << "Error: cannot select among players" << endl;
    		exit(EXIT_FAILURE);
 		}
 		//find where the potato is from
 		for(int i=0;i<num_players;i++){
 			if(FD_ISSET(player_connection_fd[i],&readfds)){
 				if(recv(player_connection_fd[i],&potato,sizeof(potato),MSG_WAITALL)!=sizeof(potato)){
 					perror("Received an broken potato at last!\n");
 				}
 				//Potato close_message;
				Potato close_message;
				for(int j=0;j<num_players;j++){
					send(player_connection_fd[j],&close_message,sizeof(close_message),0);
				}
				break;
 			}
 		}
 		//print trace
 		potato.print_trace();
 		//sleep to give players time to close
 		//usleep(100000);
	}

	~Ringmaster(){
	  //close player_connect socket
	  for(int i=0;i<num_players;i++){
	    close(player_connection_fd[i]);
	  }
	  //close ringmaster
	  close(ringmaster_fd);
	}

};

int main(int argc, char ** argv){
	if (argc < 4){
    std::cout << "Usage: ./ringmaster <port_num> <num_players> <num_hops>\n";
	}
  	Ringmaster *ringmaster = new Ringmaster(argv);

  	ringmaster->init_print();
  	ringmaster->get_address_info();
  	ringmaster->create_socket();
  	ringmaster->establish_connections();
  	ringmaster->send_circle_info();
  	ringmaster->play_game();
	delete ringmaster;
  	return 0;
}























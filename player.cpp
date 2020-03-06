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
class Player{
public:
  int num_players;
  int player_ID;
  int master_fd;//client for ringmaster
  int server_fd;//server socket fd
  int server_accept_fd;//server accept fd
  int client_connect_fd;//client connect fd
  Player():num_players(0),player_ID(0),master_fd(0){}

  void player_connect_master(char ** argv){
    //define variables for conncetion with ring master
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char * hostname;
    const char * port;
    hostname = argv[1];
    port = argv[2];
    //initialize
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    //get address info
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(EXIT_FAILURE);
    } 
    //create socket
    master_fd = socket(host_info_list->ai_family, 
		       host_info_list->ai_socktype, 
		       host_info_list->ai_protocol);
    if (master_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(EXIT_FAILURE);
    }
    //clear the socket
    char buffer[512];
    memset(buffer,0,512);
    read(master_fd,buffer,512);
    memset(buffer,0,512);
    //set up connection
    status = connect(master_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(EXIT_FAILURE);
    }
    recv(master_fd,&player_ID,sizeof(player_ID),MSG_WAITALL);
    recv(master_fd,&num_players,sizeof(num_players),MSG_WAITALL);
    printf("Connected as player %d out of %d total players\n",player_ID,num_players);
    freeaddrinfo(host_info_list);
  }

   void player_build_server(){
      int status;
      struct addrinfo host_info;
      struct addrinfo *host_info_list;
      const char *hostname = NULL;
      const char *port = "0";
      //omit port here
      //init
      memset(&host_info, 0, sizeof(host_info));
      host_info.ai_family   = AF_UNSPEC;
      host_info.ai_socktype = SOCK_STREAM;
      host_info.ai_flags    = AI_PASSIVE;
      //get address info
      status = getaddrinfo(hostname, port, &host_info, &host_info_list);
      if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      }
      //ask os to assign the port
      struct sockaddr_in *addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
      addr_in->sin_port = 0;
      //create socket
      server_fd = socket(host_info_list->ai_family, 
         host_info_list->ai_socktype, 
         host_info_list->ai_protocol);
      if (server_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      }
      //bind
      int yes = 1;
      status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
      status = bind(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
      if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      } 
      //listen
      status = listen(server_fd, 100);
      if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      } 
      //get the server's port
      struct sockaddr_in serverAddr;
      socklen_t serverAddrLen;
      serverAddrLen = sizeof(serverAddr);
      getsockname(server_fd, (struct sockaddr *)&serverAddr, &serverAddrLen);
      int port_num = ntohs(serverAddr.sin_port);
      send(master_fd,&port_num,sizeof(port_num),0);
      freeaddrinfo(host_info_list);
   }

  void player_connect_circle(){
      char neighbour_IP[100];
      memset(&neighbour_IP,0,100);
      int neighbour_port;
      int IP_SIZE;
      //receive neighbour's port number from ringmaster
      recv(master_fd,&IP_SIZE,sizeof(IP_SIZE),MSG_WAITALL);
      recv(master_fd,neighbour_IP,IP_SIZE,MSG_WAITALL);
      neighbour_IP[IP_SIZE]='\0';
      recv(master_fd,&neighbour_port,sizeof(neighbour_port),MSG_WAITALL);
      //become the client of the neighbour
      int status;
      struct addrinfo host_info;
      struct addrinfo *host_info_list;
      const char *hostname = neighbour_IP;
      const char *port     = (char*)std::to_string(neighbour_port).c_str();
      memset(&host_info, 0, sizeof(host_info));
      host_info.ai_family   = AF_UNSPEC;
      host_info.ai_socktype = SOCK_STREAM;
      //getaddrinfo
      status = getaddrinfo(hostname, port, &host_info, &host_info_list);
      if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      }
      //socket
      client_connect_fd = socket(host_info_list->ai_family, 
         host_info_list->ai_socktype, 
         host_info_list->ai_protocol);
      if (client_connect_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      }
      //clear the socket
      char buffer[512];
      memset(buffer,0,512);
      read(client_connect_fd,buffer,512);
      memset(buffer,0,512);
      //connect
      status = connect(client_connect_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
      if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
      }
      //each player send a message to the ringmaster mean that the circle connction is ready
      int num = 1;
      send(master_fd,&num,sizeof(num),0);
      //enter accpet
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      server_accept_fd = accept(server_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
      if (server_accept_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        exit(EXIT_FAILURE);
      }
      freeaddrinfo(host_info_list);
    }

    void throw_potato(){
      //initialize for select
      fd_set readfds;
      // Potato potato;
      int client_connect = (player_ID+1)%num_players;
      int server_accept = (player_ID-1+num_players)%num_players;
      //srand((unsigned int)time(NULL)+player_ID);
      int bigger = client_connect_fd > server_accept_fd ? client_connect_fd : server_accept_fd;
      int fdmax = master_fd > bigger ? master_fd : bigger;
      while(true){
	FD_ZERO(&readfds);
        FD_SET(master_fd,&readfds);
        FD_SET(client_connect_fd,&readfds);
        FD_SET(server_accept_fd,&readfds);
	Potato potato;
        //generate a random number
        int random = rand() % 2;
        //select section
        int temp = select(fdmax+1,&readfds,NULL,NULL,NULL);
        if(temp == -1){
          cerr << "Error: cannot select to send potato among players" << endl;
          exit(EXIT_FAILURE);
        }
        //1.master_fd receive
        if(FD_ISSET(master_fd,&readfds)){
          recv(master_fd,&potato,sizeof(potato),0);
	  //handle the situation that hops=0(end game message)
          if(potato.hops == 0){
            break;
          }
          //normal situation
          potato.hops = potato.hops - 1;
          potato.trace[potato.count]=player_ID;
          potato.count = potato.count + 1;
          if(potato.hops<=0){
            printf("I'm it\n");
            send(master_fd,&potato,sizeof(potato),0);
	    //sleep(1);
            continue;
          }
          else{
            if(random == 0){
              send(client_connect_fd,&potato,sizeof(potato),0);
              printf("Sending potato to %d\n",client_connect);
              continue;
            }
            if(random == 1){
              send(server_accept_fd,&potato,sizeof(potato),0);
              printf("Sending potato to %d\n",server_accept);
              continue;
            }
          }
        }
        //2.client_connect_fd receive the potato
        else if(FD_ISSET(client_connect_fd,&readfds)){
	  recv(client_connect_fd,&potato,sizeof(potato),0);
	  if(potato.isreal!=true){
	    continue;//debug
	  }
	  if(potato.hops==0){
	    continue;//debug
	  }
	  potato.hops--;
          potato.trace[potato.count]=player_ID;
          potato.count++;
          if(potato.hops<=0){
            printf("I'm it\n");
            send(master_fd,&potato,sizeof(potato),0);
	    //sleep(1);
            continue;
          }
          else{
            if(random == 0){
              send(client_connect_fd,&potato,sizeof(potato),0);
              printf("Sending potato to %d\n",client_connect);
              continue;
            }
            if(random == 1){
              send(server_accept_fd,&potato,sizeof(potato),0);
	      printf("Sending potato to %d\n",server_accept);
              continue;
            }
          } 
        }
        //3. server_accept_fd receive the potato
        else{
          recv(server_accept_fd,&potato,sizeof(potato),0);
	  if(potato.isreal!=true){
            continue;//debug
          }
	  if(potato.hops==0){
	    continue;//debug
	  }
          potato.hops--;
          potato.trace[potato.count]=player_ID;
          potato.count++;
          if(potato.hops<=0){
            printf("I'm it\n");
            send(master_fd,&potato,sizeof(potato),0);
	    // sleep(1);
            continue;
          }
          else{
            if(random == 0){
              send(client_connect_fd,&potato,sizeof(potato),0);
              printf("Sending potato to %d\n",client_connect);
              continue;
            }
            if(random == 1){
              send(server_accept_fd,&potato,sizeof(potato),0);
              printf("Sending potato to %d\n",server_accept);
              continue;
            }
          }
        }
      }
      //usleep(100000);
    }

  ~Player(){
    close(client_connect_fd);
    close(server_accept_fd);
    close(server_fd);
    close(master_fd);
   }

};




int main(int argc, char ** argv){
    if (argc < 3) {
      cout << "Syntax: client <hostname>\n" << endl;
      return 1;
    }
    Player *player = new Player();
  	
    player->player_connect_master(argv);
    
    player->player_build_server();

    player->player_connect_circle();

    player->throw_potato();

    delete player;
    return 0;
}












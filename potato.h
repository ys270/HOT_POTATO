#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include<vector>

using namespace std;

class Potato{
public:
  bool isreal;//handle errors
  int hops;
  int count;
  short trace[512];
  //constructor
 Potato() : isreal(true),hops(0),count(0){
    memset(trace,0,sizeof(trace));
  }
  //print the trace of the potato object
  void print_trace(){
    printf("Trace of potato:\n");
    for(int i=0;i<count-1;i++){
      printf("%d, ",trace[i]);
    }
    printf("%d\n",trace[count-1]);
  }
};


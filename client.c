#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>


int main(int argc, char *argv[]){

  char message[256];
  char buffer[256];
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  // Create the socket. 
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  //Configure settings of the server address
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(1100);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    addr_size = sizeof serverAddr;
    
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);       
    while (1) {
      int l;
      printf("Wait to room number\n");
      scanf("%d", &l);
      if(send(clientSocket, &l, sizeof(l), 0) < 0) printf("Send failed\n");
      int czyPokOk;
      if(recv(clientSocket, &czyPokOk, sizeof(czyPokOk), 0) < 0) printf("Receive failed\n");
      if(czyPokOk == -1) {
        printf("Pokój zajęcty\n");
        continue;
      }
      if(czyPokOk == -2) {
        printf("Pokój niepoprawny\n");
        continue;
      }
      if(czyPokOk == -3) {
        printf("Error\n");
        continue;
      }
      int ruch;
      if(recv(clientSocket, &ruch, sizeof(ruch), 0) < 0) printf("Receive failed\n");
      /*
        ruch == 0 - zaczyna
        ruch == 1 - rywal zaczyna
      */
      int start = 1;
      if(send(clientSocket, &start, sizeof(start), 0) < 0) printf("Send failed\n");
      /*
        l >= 0 - numer pokoju
        l == -1 - pokój przepełniony
        l == -2 - błędny numer pokoju
        l == -3 - błąd
      */
     
      printf("Data received: %d\n", start);
    //   memset(&message, 0, sizeof (message));
              
    }
    while(1) {;}
    close(clientSocket);


  return 0;
}

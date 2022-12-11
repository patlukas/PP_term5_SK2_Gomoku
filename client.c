#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>


int gomoku(int socket, int room, int ruch) {
  /*
    result
      4 - wygrana przez poddanie
      3 - wygrana
      2 - remis
      1 - przegrana
      -1 - error
  */
  while(1) {
    if(ruch == 0) {
      int pole;
      printf("Podaj pole: \n");
      scanf("%d", &pole);
      if(send(socket, &room, sizeof(room), 0) <= 0) {
        printf("Problem z połączeniem (1)\n");
        return -1;
      }
      int poleOk;
      if(recv(socket, &poleOk, sizeof(poleOk), 0) <= 0) {
        printf("Problem z połączeniem (2)\n");
        return -1;
      }
      if(poleOk == -1) continue;
      int ruchResult;
      if(recv(socket, &ruchResult, sizeof(ruchResult), 0) <= 0) {
        printf("Problem z połączeniem (3)\n");
        return -1;
      }
      printf("Ruch result: %d\n", ruchResult);
      if(ruchResult > 0) return ruchResult;
      ruch = 1;
    }
    else {
      int ruchRywala;
      if(recv(socket, &ruchRywala, sizeof(ruchRywala), 0) <= 0) {
        printf("Problem z połączeniem (4)\n");
        return -1;
      }
      printf("Ruch rywala: %d\n", ruchRywala);
      if(ruchRywala == -1) return 4;
      int ruchResult;
      if(recv(socket, &ruchResult, sizeof(ruchResult), 0) <= 0) {
        printf("Problem z połączeniem (5)\n");
        return -1;
      }
      printf("Ruch rywala result: %d\n", ruchResult);
      if(ruchResult > 0) return ruchResult;
      ruch = 0;
    }
  }
  return -1;
}

int main(int argc, char *argv[]) {

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
      int room;
      printf("Wait to room number\n");
      scanf("%d", &room);
      if(send(clientSocket, &room, sizeof(room), 0) <= 0) printf("Send failed\n");
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
     int result = gomoku(clientSocket, room, ruch);
      printf("Data received: %d\n", result);
    //   memset(&message, 0, sizeof (message));
              
    }
    while(1) {;}
    close(clientSocket);


  return 0;
}

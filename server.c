#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#include <unistd.h> 
#include<pthread.h>

#define NUMBEROFROOMS 3
//gcc program.c -lpthread -Wall -o outfile Remember to link pthread bib

int listRoom[3][2] = {{-1, -1}, {-1, -1}, {-1, -1}};


int sendInfoAndCatchException(int socket, int sendVal, char errorMessage[], int idClientDisconnect, int pthreadExitWhenError) {
    int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
    if(resultSend <= 0) {
        if(resultSend == -1) printf("Error send: %s\n", errorMessage);
        else if(idClientDisconnect > 0) printf("%d: Klient się rozłączył (%d)\n", socket, idClientDisconnect);
        if(pthreadExitWhenError) pthread_exit(NULL);
    }
    return resultSend;
}

int resvDataAndCatchException(int socket, char errorMessage[], int idClientDisconnect) {
    int data;
    int resultRecv = recv(socket, &data, sizeof(data), 0);
    if(resultRecv <= 0) {
        if(resultRecv == -1) printf("Error recv: %s\n", errorMessage);
        else if(idClientDisconnect > 0) printf("%d: Klient się rozłączył (%d)\n", socket, idClientDisconnect);
    }
    return resultRecv;
}

int reservingPlaceInRoom(int socket, int numberRoom, int numberPlace) {
    if(listRoom[numberRoom][numberPlace] == -1) {
        sendInfoAndCatchException(socket, 1, "Błąd podczas próby zaakceptowania pokoju", 1, 1);
        listRoom[numberRoom][numberPlace] = socket;
        int oppositePlace = 1 - numberPlace;
        int oppositeSocket = listRoom[numberRoom][oppositePlace];
        if(oppositeSocket != -1) {
            int resultSend = sendInfoAndCatchException(oppositeSocket, 1, "Błąd podczas wysyłania informacji do 1. socketu że jest rywal", -1, 0);
            if(resultSend <= 0) {
                listRoom[numberRoom][oppositePlace] = -1;
                if(resultSend == -1) close(oppositeSocket);
            }
            else {
                if(sendInfoAndCatchException(socket, 1, "Błąd podczas wysyłania informacji do 2. socketu że jest rywal", 2, 0) <= 0) {
                    listRoom[numberRoom][numberPlace] = -1;
                    pthread_exit(NULL);
                }
            }
        }
        if(resvDataAndCatchException(socket, "Błąd podczas kończenia wyboru pokoju (A)", 4) <= 0) {
            listRoom[numberRoom][numberPlace] = -1;
            pthread_exit(NULL);
        }
        return 1;
    }
    else return -1;
}

int roomSelection(int socket) {
    while(1) {
        int selectedRoom;
        int resultRecv = recv(socket, &selectedRoom, sizeof(selectedRoom), 0);
        if(resultRecv <= 0) {
            if(resultRecv == -1) printf("Error recv: Błąd podczas odczytywania numeru pokoju\n");
            else printf("%d: Klient się rozłączył (6)\n", socket);
            pthread_exit(NULL);
        }
        if(selectedRoom < 0 || selectedRoom >= NUMBEROFROOMS) {
            sendInfoAndCatchException(socket, -2, "Błąd podczas wysyłania informcji o błędnym numerze pokoju", 7, 1);
        }
        printf("%d: wybrał %d pokój\n", socket, selectedRoom);
        int reservingResult = reservingPlaceInRoom(socket, selectedRoom, 0);
        if(reservingResult == -1) reservingResult = reservingPlaceInRoom(socket, selectedRoom, 1);
        if(reservingResult == -1) {
            printf("%d: Pokój zajęty\n", socket);
            sendInfoAndCatchException(socket, -1, "Błąd podczas wysyłania informcji o zajętym pokoju", 8, 1);
        }
        else return selectedRoom;
    }
    return -1;
}

void * clinetThread(void *arg) {
    int newSocket = *((int *)arg);
    printf("%d: Nr socket: %d\n", newSocket, newSocket);
    int selectedRoom = roomSelection(newSocket);
    
    printf("%d: przydział do pokoju %d\n",newSocket, selectedRoom);
    while(1) {;}
    pthread_exit(NULL);
}

int createServerSocket() {
    int serverSocket;
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1) {
        printf("Error socket: Błąd podczas tworzenia gniazda\n");
        exit (EXIT_FAILURE);
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1100);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int resultBind = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if(resultBind == -1) {
        printf("Error bind: Błąd podczas przypisywania adresu do gniazda\n");
        exit (EXIT_FAILURE);
    }

    if(listen(serverSocket, 10) == 0) printf("Serwer oczekuje na połączenie...\n");
    else {
        printf("Error listen: Błąd podczas ustawiania gniazda jako giazdo pasywne\n");
        exit (EXIT_FAILURE);
    }
    
    return serverSocket;
}

int createConnect(int serverSocket) {
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    addr_size = sizeof serverStorage;
    int newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
    if(newSocket == -1) {
        printf("Error accept: Błąd podczas akceptowania połączenia\n");
        exit (EXIT_FAILURE);
    }
    return newSocket;
}

void createThreadToClientConnect(int socket) {
    pthread_t thread_id;
    if( pthread_create(&thread_id, NULL, clinetThread, &socket) != 0) printf("Error pthread_create: Błąd podczas tworzenia wątku\n");
    pthread_detach(thread_id);
}

int main(){
    int serverSocket = createServerSocket();
    while(1) {
        int newSocket = createConnect(serverSocket);
        createThreadToClientConnect(newSocket);
    }
    return 0;
}

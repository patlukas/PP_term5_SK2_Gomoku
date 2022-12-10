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

char client_message[256];
char buffer[256];
int listRoom[3][2] = {{-1, -1}, {-1, -1}, {-1, -1}};


int reservingPlaceInRoom(int socket, int numberRoom, int numberPlace) {
    if(listRoom[numberRoom][numberPlace] == -1) {
        listRoom[numberRoom][numberPlace] = socket;
        int sendVal = 1;
        int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
        if(resultSend == -1) {
            printf("Error send: Błąd podczas próby zaakceptowania pokoju\n");
            return -2;
        }
        else if(resultSend == 0) {
            listRoom[numberRoom][numberPlace] = -1;
            printf("%d: Klient się rozłączył (1)\n", socket);
            pthread_exit(NULL);
        }
        int oppositePlace = 1 - numberPlace;
        int oppositeSocket = listRoom[numberRoom][oppositePlace];
        if(oppositeSocket != -1) {
            int sendVal = 1;
            int resultSend = send(oppositeSocket, &sendVal, sizeof(sendVal), 0);
            if(resultSend == 0) listRoom[numberRoom][oppositePlace] = -1;
            else if(resultSend == -1) {
                printf("Error send: Błąd podczas wysyłania informacji do 1. socketu że jest rywal\n");
                return -2;
            }
            else {
                resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
                if(resultSend == -1) {
                    printf("Error send: Błąd podczas wysyłania informacji do 2. socketu że jest rywal\n");
                }
                else if(resultSend == 0) {
                    listRoom[numberRoom][numberPlace] = -1;
                    printf("%d: Klient się rozłączył (1)\n", socket);
                    pthread_exit(NULL);
                }
            }
            int m;
            recv(socket, &m, sizeof(m), 0);
        } else {
            int m;
            int resultRecv = recv(socket, &m, sizeof(m), 0);
            if(resultRecv == -1) {
                printf("Error recv: Błąd podczas otrzymywania potwierdzenia\n");
                return -1;
            }
            else if(resultRecv == 0) {
                listRoom[numberRoom][numberPlace] = -1;
                printf("%d: Klient się rozłączył (2)\n", socket);
                pthread_exit(NULL);
            }
            
        }
        return 1;
    }
    else return -1;
}

int roomSelection(int socket) {
    while(1) {
        int selectedRoom;
        int resultRecv = recv(socket, &selectedRoom, sizeof(selectedRoom), 0);
        if(resultRecv == -1) {
            printf("Error recv: Błąd podczas odczytywania numeru pokoju\n");
            int sendVal = -3;
            int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
            if(resultSend == -1) {
                printf("Error send: Błąd podczas wysyłania informcji o błędzie\n");
            }
            else if(resultSend == 0) {
                printf("%d: Klient się rozłączył (4)\n", socket);
                pthread_exit(NULL);
            }
            continue;
        }
        else if(resultRecv == 0) {
            printf("%d: Klient się rozłączył (3)\n", socket);
            pthread_exit(NULL);
        }
        if(selectedRoom < 0 || selectedRoom >= NUMBEROFROOMS) {
            int sendVal = -2;
            int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
            if(resultSend == -1) {
                printf("Error send: Błąd podczas wysyłania informcji o błędnym numerze pokoju\n");
            }
            else if(resultSend == 0) {
                printf("%d: Klient się rozłączył (5)\n", socket);
                pthread_exit(NULL);
            }
            continue;
        }
        printf("%d: wybrał %d pokój\n", socket, selectedRoom);
        int reservingResult = reservingPlaceInRoom(socket, selectedRoom, 0);
        if(reservingResult == -1) {
            reservingResult = reservingPlaceInRoom(socket, selectedRoom, 1);
        }
        if(reservingResult == -1) {
            printf("%d: Pokój zajęty\n", socket);
            int sendVal = -1;
            int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
            if(resultSend == -1) {
                printf("Error send: Błąd podczas wysyłania informcji o zajętym pokoju\n");
            }
            else if(resultSend == 0) {
                printf("%d: Klient się rozłączył (6)\n", socket);
                pthread_exit(NULL);
            }
            continue;
        }
        else if(reservingResult == -2) {
            int sendVal = -3;
            int resultSend = send(socket, &sendVal, sizeof(sendVal), 0);
            if(resultSend == -1) {
                printf("Error send: Błąd podczas wysyłania informcji o błędzie w reservingPlaceInRoom\n");
            }
            else if(resultSend == 0) {
                printf("%d: Klient się rozłączył (7)\n", socket);
                pthread_exit(NULL);
            }
            continue;
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
    while(1) {
        int l;
        recv(newSocket, &l, sizeof(l), 0);

        printf("%d", l);
        char* message = "message";
        send(newSocket, message, sizeof(message), 0);
        if (!strcmp(message, "koniec")) break;
        memset(&client_message, 0, sizeof (client_message));
    }
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

    if(listen(serverSocket, 10) == 0) {
        printf("Serwer oczekuje na połączenie...\n");
    }
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
    if( pthread_create(&thread_id, NULL, clinetThread, &socket) != 0) {
        printf("Error pthread_create: Błąd podczas tworzenia wątku\n");
    }
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

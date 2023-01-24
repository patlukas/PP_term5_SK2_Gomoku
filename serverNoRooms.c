#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <pthread.h>

struct game {
    int * room;
    int * gomokuBoards;
};

struct game kolejka;

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

void createGomokuBoard(struct game g) {
    for(int i=0; i<225; i++) g.gomokuBoards[i] = -1;
}

int reservingPlaceInRoom(int socket, struct game g, int numberPlace) {
    printf("%d %d\n", g.room[0], g.room[1]);
    if(g.room[numberPlace] == -1) {
        sendInfoAndCatchException(socket, 1, "Błąd podczas próby zaakceptowania pokoju", 1, 1);
        g.room[numberPlace] = socket;
        int oppositePlace = 1 - numberPlace;
        int oppositeSocket = g.room[oppositePlace];
        printf("%d %d\n", g.room[0], g.room[1]);
        if(oppositeSocket != -1) {
            int resultSend = sendInfoAndCatchException(oppositeSocket, 1, "Błąd podczas wysyłania informacji do 1. socketu że jest rywal", -1, 0);
            if(resultSend <= 0) {
                g.room[oppositePlace] = -1;
                if(resultSend == -1) close(oppositeSocket);
            }
            else {
                if(sendInfoAndCatchException(socket, 2, "Błąd podczas wysyłania informacji do 2. socketu że jest rywal", 2, 0) <= 0) {
                    g.room[numberPlace] = -1;
                    pthread_exit(NULL);
                }
                createGomokuBoard(g);
            }
        }
        if(resvDataAndCatchException(socket, "Błąd podczas kończenia wyboru pokoju (A)", 4) <= 0) {
            g.room[numberPlace] = -1;
            pthread_exit(NULL);
        }
        return 1;
    }
    else return -1;
}

int checkPole(struct game g, int pole, int socket) {
    if(pole < 0 || pole >= 225) return -1;
    if(g.gomokuBoards[pole] == -1) {
        g.gomokuBoards[pole] = socket;
        return 1;
    }
    return -1;
}

int getGameResult(struct game g, int socket) {
    /*
        sprawdza ułożenie pól i zwraca
        1 - brak rozstrzygnięcia
        2 - przegrana
        3 - remis
        4 - wygrana
    */
   //sprawdzanie wygranej w linii poziomej

    for(int i=0; i<15; i++) {
        int lenX = 0;
        int lenY = 0;
        for(int j=0; j<15; j++) {
            if(g.gomokuBoards[i*15+j] == socket) lenX += 1;
            else {
                if(lenX == 5) return 4;
                else lenX = 0;
            }
            if(j == 14 && lenX == 5) return 4;

            if(g.gomokuBoards[j*15+i] == socket) lenY += 1;
            else {
                if(lenY == 5) return 4;
                else lenY = 0;
            }
            if(j == 14 && lenY == 5) return 4;
        }
    }

    //spr przekątnej 1.
    for(int i=-10; i<=10; i++) {
        int x = i;
        int y = 0;
        int len = 0;
        while(x < 0) {
            x += 1;
            y += 1;
        }
        while(x < 15 && y < 15) {
            if(g.gomokuBoards[y*15 + x] == socket) len += 1;
            else {
                if(len == 5) return 4;
                else len = 0;
            }
            if((x + 1 == 15 || y + 1 == 15) && len == 5) return 4;
            x += 1;
            y += 1;
        }
    }
    //spr przekątnej 2.
    for(int i=4; i<=24; i++) {
        int x = i;
        int y = 0;
        int len = 0;
        while(x > 14) {
            x -= 1;
            y += 1;
        }
        while(x >= 0 && y < 15) {
            if(g.gomokuBoards[y*15 + x] == socket) len += 1;
            else {
                if(len == 5) return 4;
                else len = 0;
            }
            if((x - 1 == -1 || y + 1 == 15) && len == 5) return 4;
            x -= 1;
            y += 1;
        }
    }
    for(int i=0; i<225; i++) {
        if(g.gomokuBoards[i] == -1) return 1;
    }
    return 3;
}

int gomokuGame_sendOppositeSocketAfterError(int oppositeSocket, int val, struct game g, int placeInRoom, int socket, int nrError) {
    printf("%d: Problem z połączeniem i zwolniono miejsce (%d)\n", socket, nrError);
    send(oppositeSocket, &val, sizeof(val), 0);
    g.room[placeInRoom] = -1;
    close(socket);
    pthread_exit(NULL);
}

int gomokuGame(int socket, struct game g) {
    /*
        return:
            -1 - error
            5 - wygrana przez pddanie
            3 - remis
            4 - wygrana
    */
    int placeInRoom = 1;
    int oppositeSocket = g.room[0];
    if(oppositeSocket == socket) {
        placeInRoom = 0;
        oppositeSocket = g.room[1];
    }
    if(oppositeSocket == -1) {
        printf("Error z pokojem: jedno z miejsc jest puste\n");
        return -1;
    }
    while(1) {
        int pole;
        if(recv(socket, &pole, sizeof(pole), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, -2, g, placeInRoom, socket, 1);
        int poleOk = checkPole(g, pole, socket);
        if(send(socket, &poleOk, sizeof(poleOk), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, -2, g, placeInRoom, socket, 2);
        if(poleOk == -1) continue;
        int poleToSend = pole + 1;
        if(send(oppositeSocket, &poleToSend, sizeof(poleToSend), 0) <= 0) {
            printf("%d: Brak połączenia z rywalem więc koniec rozgrywki\n", socket);
            g.room[placeInRoom] = -1;
            int val = 5;
            if(send(socket, &val, sizeof(val), 0) <= 0) {
                printf("%d: Problem z połączeniem, więc koniec rozgrywki i zwolniono miejsce (4)\n", socket);
                pthread_exit(NULL);
            };
            return 5;
        }
        int result = getGameResult(g, socket);
        if(send(socket, &result, sizeof(result), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, 5, g, placeInRoom, socket, 3);
        int oppositeResult = result;
        if(result == 4) oppositeResult = 2;
        send(oppositeSocket, &oppositeResult, sizeof(oppositeResult), 0);
        if(result > 1) {
            g.room[placeInRoom] = -1;
            return result;
        }
    }
}

void * clinetThread(void *arg) {
    int newSocket = *((int *)arg);
    printf("%d: Nr socket: %d\n", newSocket, newSocket);

    struct game g;
    g.room = kolejka.room;
    g.gomokuBoards = kolejka.gomokuBoards;
    int reservingResult = reservingPlaceInRoom(newSocket, g, 0);
    if (reservingResult == -1) reservingResult = reservingPlaceInRoom(newSocket, g, 1);
    if(kolejka.room[0] != -1 && kolejka.room[1] != -1) {
        kolejka.room = (int*) malloc(2 * sizeof(int));
        kolejka.room[0] = -1;
        kolejka.room[1] = -1;
        kolejka.gomokuBoards = (int*) malloc(225 * sizeof(int));
    }
    
    int result = gomokuGame(newSocket, g);
    printf("%d: Rezultat gra: %d\n", newSocket, result);
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
    kolejka.room = (int*) malloc(2 * sizeof(int));
    kolejka.room[0] = -1;
    kolejka.room[1] = -1;
    kolejka.gomokuBoards = (int*) malloc(225 * sizeof(int));
    int serverSocket = createServerSocket();
    while(1) {
        int newSocket = createConnect(serverSocket);
        createThreadToClientConnect(newSocket);
    }
    return 0;
}

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
int listGomokuBoards[3][225];


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

void createGomokuBoard(int room) {
    for(int i=0; i<225; i++) listGomokuBoards[room][i] = -1;
}

int reservingPlaceInRoom(int socket, int numberRoom, int numberPlace) {
    if(listRoom[numberRoom][numberPlace] == -1) {
        sendInfoAndCatchException(socket, 1, "Błąd podczas próby zaakceptowania pokoju", 1, 1);
        listRoom[numberRoom][numberPlace] = socket;
        int oppositePlace = 1 - numberPlace;
        int oppositeSocket = listRoom[numberRoom][oppositePlace];
        if(oppositeSocket != -1) {
            int resultSend = sendInfoAndCatchException(oppositeSocket, 0, "Błąd podczas wysyłania informacji do 1. socketu że jest rywal", -1, 0);
            if(resultSend <= 0) {
                listRoom[numberRoom][oppositePlace] = -1;
                if(resultSend == -1) close(oppositeSocket);
            }
            else {
                if(sendInfoAndCatchException(socket, 1, "Błąd podczas wysyłania informacji do 2. socketu że jest rywal", 2, 0) <= 0) {
                    listRoom[numberRoom][numberPlace] = -1;
                    pthread_exit(NULL);
                }
                createGomokuBoard(numberRoom);
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
            printf("%d: wybrał %d pokój, a jest to niepoprawny numer\n", socket, selectedRoom);
            sendInfoAndCatchException(socket, -2, "Błąd podczas wysyłania informcji o błędnym numerze pokoju", 7, 1);
            continue;
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

int checkPole(int room, int pole, int socket) {
    if(pole < 0 || pole >= 225) return -1;
    if(listGomokuBoards[room][pole] == -1) {
        listGomokuBoards[room][pole] = socket;
        return 0;
    }   
    return -1;
}

int getGameResult(int room, int socket) {
    /*
        sprawdza ułożenie pól i zwraca 
        0 - brak rozstrzygnięcia
        1 - przegrana
        2 - remis
        3 - wygrana
    */
   //sprawdzanie wygranej w linii poziomej
   
    for(int i=0; i<15; i++) {
        int lenX = 0;
        int lenY = 0;
        for(int j=0; j<15; j++) {
            if(listGomokuBoards[room][i*15+j] == socket) lenX += 1;
            else {
                if(lenX == 5) return 3;
                else lenX = 0;
            }
            if(j == 14 && lenX == 5) return 3;

            if(listGomokuBoards[room][j*15+i] == socket) lenY += 1;
            else {
                if(lenY == 5) return 3;
                else lenY = 0;
            }
            if(j == 14 && lenY == 5) return 3;
        }
    }
    //spr przekątnej 1.
    for(int i=0; i<=10; i++) {
        int j = i;
        int len = 0;
        while(j < 225) {
            if(listGomokuBoards[room][j] == socket) len += 1;
            else {
                if(len == 5) return 3;
                else len = 0;
            }
            if(j + 16 >= 225 && len == 5) return 3;
            j += 16;
        }
        j = i*15;
        len = 0;
        while(j < 225) {
            if(listGomokuBoards[room][j] == socket) len += 1;
            else {
                if(len == 5) return 3;
                else len = 0;
            }
            if(j + 16 >= 225 && len == 5) return 3;
            j += 16;
        }
        
        j = 210 + i;
        len = 0;
        while(j > 0) {
            if(listGomokuBoards[room][j] == socket) len += 1;
            else {
                if(len == 5) return 3;
                else len = 0;
            }
            if(j - 14 < 0 && len == 5) return 3;
            j -= 14;
        }
        j = (14 - i) * 15;
        len = 0;
        while(j > 0) {
            if(listGomokuBoards[room][j] == socket) len += 1;
            else {
                if(len == 5) return 3;
                else len = 0;
            }
            if(j - 14 < 0 && len == 5) return 3;
            j -= 14;
        }
    }
    for(int i=0; i<225; i++) {
        if(listGomokuBoards[room][i] == -1) return 0;
    }
    return 2;
}

int gomokuGame_sendOppositeSocketAfterError(int oppositeSocket, int val, int room, int placeInRoom, int socket, int nrError) {
    printf("%d: Problem z połączeniem, więc koniec rozgrywki w pokoju %d i zwolniono miejsce (%d)\n", socket, room, nrError);
    send(oppositeSocket, &val, sizeof(val), 0);
    listRoom[room][placeInRoom] = -1;
    pthread_exit(NULL);
}

int gomokuGame(int socket, int room) {
    /*
        return:
            -1 - error
            4 - wygrana przez pddanie
            2 - remis
            3 - wygrana
    */
    int placeInRoom = 1;
    int oppositeSocket = listRoom[room][0];
    if(oppositeSocket == socket) {
        placeInRoom = 0;
        oppositeSocket = listRoom[room][1];
    }
    if(oppositeSocket == -1) {
        printf("Error z pokojem: jedno z miejsc jest puste\n");
        return -1;
    }
    while(1) {
        int pole;
        if(recv(socket, &pole, sizeof(pole), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, -1, room, placeInRoom, socket, 1);
        int poleOk = checkPole(room, pole, socket);
        if(send(socket, &poleOk, sizeof(poleOk), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, -1, room, placeInRoom, socket, 2);
        if(poleOk == -1) continue;
        if(send(oppositeSocket, &pole, sizeof(pole), 0) <= 0) {
            printf("%d: Brak połączenia z rywalem więc koniec rozgrywki w pokoju %d\n", socket, room);
            listRoom[room][placeInRoom] = -1;
            int val = 4;
            if(send(socket, &val, sizeof(val), 0) <= 0) {
                printf("%d: Problem z połączeniem, więc koniec rozgrywki w pokoju %d i zwolniono miejsce (4)\n", socket, room);
                pthread_exit(NULL);
            };
            return 4;
        }
        int result = getGameResult(room, socket);
        if(send(socket, &result, sizeof(result), 0) <= 0) gomokuGame_sendOppositeSocketAfterError(oppositeSocket, 4, room, placeInRoom, socket, 3);
        int oppositeResult = result;
        if(result == 3) oppositeResult = 1;
        send(oppositeSocket, &oppositeResult, sizeof(oppositeResult), 0);
        if(result > 0) {
            listRoom[room][placeInRoom] = -1;
            return result;
        }
    }
}

void * clinetThread(void *arg) {
    int newSocket = *((int *)arg);
    printf("%d: Nr socket: %d\n", newSocket, newSocket);
    int selectedRoom = roomSelection(newSocket);
    
    printf("%d: przydział do pokoju %d\n",newSocket, selectedRoom);
    int result = gomokuGame(newSocket, selectedRoom);
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
    int serverSocket = createServerSocket();
    while(1) {
        int newSocket = createConnect(serverSocket);
        createThreadToClientConnect(newSocket);
    }
    return 0;
}

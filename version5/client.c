#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include "message.h"
#define BUFFER_SIZE 1024
#define SERVER "127.0.0.1"
#define PORTS "2058"
#define PORTC "4096"

// Signal handler pour sigint
void sigintHandler(int _);

int parseResponse(char *buffer, Response *rep); 

int clientSockFd;


int main() {
    // Initialisation des variables
    struct addrinfo hints, *clientInfo, *serverInfo;
    int showIdx, nbPlace;
    Response response;
    char showIdxMap[BUFFER_SIZE];
    int requestNb, statusCode;
    char recvBuffer[BUFFER_SIZE];
    char sendBuffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];

    const size_t recvBufferSize = sizeof(recvBuffer);
    const size_t sendBufferSize = sizeof(sendBuffer);
    const size_t msgSize = sizeof(msg);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, PORTC, &hints, &clientInfo) == -1) {
        fprintf(stderr, "LOCAL GETADDRINFO FAILURE \n");
        exit(1);
    }

    clientSockFd = socket(clientInfo->ai_family, clientInfo->ai_socktype, clientInfo->ai_protocol);
    if (clientSockFd == -1) {
        fprintf(stderr, "SOCKET CREATION FAILURE \n");
        exit(1);
    }
    if (bind(clientSockFd, clientInfo->ai_addr, clientInfo->ai_addrlen) == -1) {
        fprintf(stderr, "SOCKET BINDING FAILURE \n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(SERVER, PORTS, &hints, &serverInfo) == -1) {
        fprintf(stderr, "REMOTE GETADDRINFO FAILURE \n");
        exit(1);
    }
    if (connect(clientSockFd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
        fprintf(stderr, "CONNECT TO SERVER FAILURE \n");
        exit(1);
    }

    signal(SIGINT, sigintHandler);

    printf("Requesting list of shows.\n");
    if (send(clientSockFd, "0", 1, 0) == -1) {
        fprintf(stderr, "SEND TO SERVER FAILURE");
        exit(1);
    }
    memset(recvBuffer, 0, recvBufferSize);
    if (recv(clientSockFd, recvBuffer, recvBufferSize, 0) == -1){
            fprintf(stderr, "RECV FROM CLIENT FAILURE\n");
            exit(1);
    }

    sscanf(recvBuffer, "%d %s", &statusCode, showIdxMap);

    while (1) {
        printf("Here is the list of available shows.\n");
        printf("%s\n", showIdxMap);

        // Nettoyage des buffers de réponse/requête
        memset(recvBuffer, 0, recvBufferSize);
        memset(sendBuffer, 0, sendBufferSize);
        memset(msg, 0, msgSize);

        // Demande de l'action souhaitée
        printf("What is your request?\n 1. Get remaining places for a show.\n 2. Book places for a show.\n 3. Exit \n--> ");
        fflush(stdout);
        scanf("%d", &requestNb);

        // Requête de consultation
        if (requestNb == 1) { 

            // Choix du spectacle (via son indice)
            printf("Enter show index to see remaining places: ");
            fflush(stdout);
            scanf("%d", &showIdx);

            // Préparation de la requête
            snprintf(sendBuffer, sendBufferSize, "%d %d", CHECK, showIdx);

            // Envoie de la requête
            if (send(clientSockFd, sendBuffer, sendBufferSize, 0) == -1) {
                fprintf(stderr, "SEND TO SERVER FAILURE");
                exit(1);
            }

            // Réception et affichage de la réponse
            if (recv(clientSockFd, recvBuffer, recvBufferSize, 0) == -1) {
                fprintf(stderr, "RECV FROM CLIENT FAILURE\n");
                exit(1);
            }

            sscanf(recvBuffer, "%d %s", &statusCode, msg);
            if (statusCode == SUCCESS)
            printf("Remaining places: %s\n\n", msg);
            else printf("%s\n", msg);

        } else if (requestNb == 2) {

            // Choix du spectacle et du nombre de places
            printf("Enter show index and number of places to book (ex: 1 5): ");
            fflush(stdout);
            scanf("%d %d", &showIdx, &nbPlace);

            // Préparation de la requête
            snprintf(sendBuffer, sendBufferSize, "%d %d %d", CHECK, showIdx, nbPlace);

            // Envoie de la requête
            if (send(clientSockFd, sendBuffer, sendBufferSize, 0) == -1) {
                fprintf(stderr, "SEND TO SERVER FAILURE");
                exit(1);
            }

            // Réception et affichage de la réponse
            if (recv(clientSockFd, recvBuffer, recvBufferSize, 0) == -1) {
                fprintf(stderr, "RECV FROM CLIENT FAILURE\n");
                exit(1);
            }

            sscanf(recvBuffer, "%d %s", &statusCode, msg);
            if (statusCode == SUCCESS)
            printf("Server response: %s\n\n", msg);
            else printf("%s\n", msg);

        } else if (requestNb == 3) {
            printf("Leaving communication with server... \n");

            // Préparation de la requête
            snprintf(sendBuffer, sendBufferSize, "%d", EXIT);

            // Envoie de la requête
            if (send(clientSockFd, sendBuffer, sendBufferSize, 0) == -1) {
                fprintf(stderr, "SEND TO SERVER FAILURE");
                exit(1);
            }

            // Réception et affichage de la réponse
            if (recv(clientSockFd, recvBuffer, recvBufferSize, 0) == -1) {
                fprintf(stderr, "RECV FROM CLIENT FAILURE\n");
                exit(1);
            }
            sscanf(recvBuffer, "%d %s", &statusCode, msg);
            if (statusCode == SUCCESS) {
                printf("Server response: %s\n\n", msg);
                break;
            }
            else printf("%s\n", msg);
            
        } else {
            printf("Unknown request: %d\n\n", requestNb);
        }

    }

    close(clientSockFd);
    return 0;
}



void sigintHandler(int _) {
    close(clientSockFd);
    exit(0);
}

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include "message.h"
#define BUFFER_SIZE 1024
#define SERVER "127.0.0.1"
#define PORTS "2058"

// Signal handler pour sigint
void sigintHandler(int _);

// Parse status code and copy the rest of the server reply into msg.
// Returns the status code, or -1 on parse failure.
static int parseResponse(const char *buffer, char *msg, size_t msgSize) {
    int statusCode;
    if (sscanf(buffer, "%d", &statusCode) != 1) return -1;
    const char *rest = strchr(buffer, ' ');
    if (rest && msg && msgSize > 0) {
        strncpy(msg, rest + 1, msgSize - 1);
        msg[msgSize - 1] = '\0';
        size_t len = strlen(msg);
        while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r'))
            msg[--len] = '\0';
    }
    return statusCode;
}

int clientSockFd;


int main() {
    // Initialisation des variables
    struct addrinfo hints, *serverInfo;
    int showIdx, nbPlace;
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
    if (getaddrinfo(SERVER, PORTS, &hints, &serverInfo) == -1) {
        fprintf(stderr, "REMOTE GETADDRINFO FAILURE \n");
        exit(1);
    }

    clientSockFd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (clientSockFd == -1) {
        fprintf(stderr, "SOCKET CREATION FAILURE \n");
        exit(1);
    }
    if (connect(clientSockFd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1){
        fprintf(stderr, "CONNECT TO SERVER FAILURE \n");
        exit(1);
    }
    freeaddrinfo(serverInfo);

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

    statusCode = parseResponse(recvBuffer, showIdxMap, sizeof(showIdxMap));

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

            statusCode = parseResponse(recvBuffer, msg, msgSize);
            if (statusCode == SUCCESS)
            printf("Remaining places: %s\n\n", msg);
            else printf("%s\n", msg);

        } else if (requestNb == 2) {

            // Choix du spectacle et du nombre de places
            printf("Enter show index and number of places to book (ex: 1 5): ");
            fflush(stdout);
            scanf("%d %d", &showIdx, &nbPlace);

            // Préparation de la requête
            snprintf(sendBuffer, sendBufferSize, "%d %d %d", BOOK, showIdx, nbPlace);

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

            statusCode = parseResponse(recvBuffer, msg, msgSize);
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
            statusCode = parseResponse(recvBuffer, msg, msgSize);
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

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "message.h"
#include "sync.h"
#define BUFFER_SIZE 1024

static const char PARSE_FAILURE_MSG[] = "ERROR: Request failed to be parsed by the server\n";

static int parseRequest(char *buffer, Request *req);

// Tampon global pour l'affichage de la liste des spectacles.
static char showIdxMap[BUFFER_SIZE - 2];
const size_t showIdxMapSize = sizeof(showIdxMap);


static void runService(int serviceSockFd, struct sockaddr *clientAddr) {
    char recvBuffer[BUFFER_SIZE];
    char sendBuffer[BUFFER_SIZE];
    Request request;

    const size_t recvBufferSize = sizeof(recvBuffer);
    const size_t sendBufferSize = sizeof(sendBuffer);

    bool running = true;

    while (running) {
        memset(recvBuffer, 0, recvBufferSize);
        memset(sendBuffer, 0, sendBufferSize);
        if (recv(serviceSockFd, recvBuffer, recvBufferSize, 0) == -1){
            fprintf(stderr, "RECV FROM CLIENT FAILURE\n");
            exit(1);
        }

        if (parseRequest(recvBuffer, &request) == -1){
            fprintf(stderr, "PARSE REQUEST FAILURE\n");
            send(serviceSockFd, PARSE_FAILURE_MSG, strlen(PARSE_FAILURE_MSG), 0);
            continue;
        }

        if (request.rqType == INIT) {
            printf("Client %s request to see list of shows.\n", clientAddr->sa_data);
            // Préparation message avec liste spectacles.
            snprintf(sendBuffer, sendBufferSize, "%d %s", SUCCESS, showIdxMap);
        } 
        else if (request.rqType == CHECK) {
            printf("Client %s request to see remaining place for a show.\n", clientAddr->sa_data);

            int nbPlace = checkShowSync(request.showIdx);

            if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
                // Préparation du message d'erreur
                snprintf(sendBuffer, sendBufferSize,
                        "%d Invalid index %d, index should be between 1 and %lu.\n",
                        ERROR, request.showIdx, NB_SHOWS);
                printf("Show index error, sending error message to client %s.\n",
                        clientAddr->sa_data);
            }
            else { // Si spectacle existe
                // Préparation du message avec nombre de places.
                snprintf(sendBuffer, sendBufferSize, "%d %d", SUCCESS, nbPlace);
                printf("Checking request of client %s succeeded!\n", clientAddr->sa_data);
            }
        }
        else if (request.rqType == BOOK) {
            printf("Client %s request to book places for a show.\n", clientAddr->sa_data);

            int result = bookShowSync(request.showIdx, request.nbPlace);

            if (result==-1) { // Si indice du spectacle n'existe pas:
                // Préparation du message d'erreur
                snprintf(sendBuffer, sendBufferSize,
                        "%d Invalid index %d, index should be between 1 and %lu.\n",
                        ERROR, request.showIdx, NB_SHOWS);
                printf("Show index error, sending error message to client %s.\n",
                        clientAddr->sa_data);
            } else if (result==-2) { // Si pas assez de places disponibles:
                // Préparation du message d'erreur
                snprintf(sendBuffer, sendBufferSize,
                    "%d Not enough remaining places. Remaining: %d, requestPtr->d: %d.\n",
                    ERROR, checkShowSync(request.showIdx), request.nbPlace);
                printf("Not enough places error, sending error message to client.\n");
            } else { // Le spectacle existe et le nombre de places est suffisant
                // Préparation du message de confirmation.
                snprintf(sendBuffer, sendBufferSize,
                    "%d %d places booked for %s", SUCCESS, result, SHOWS[request.showIdx - 1].title);
                printf("Booking request of Client %s succeeded!\n", clientAddr->sa_data);
            }
        }
        else if (request.rqType == EXIT) {
            printf("Client %s is leaving communication", clientAddr->sa_data);
            snprintf(sendBuffer, sendBufferSize, "%d Bye!\n", SUCCESS);
            running = false;
        }
        if (send(serviceSockFd, sendBuffer, sendBufferSize, 0) == -1) {
            fprintf(stderr, "SEND TO CLIENT FAILURE");
            exit(1);
        }
    }
}


static int parseRequest(char *buffer, Request *req) {
    // Initialiser les champs optionnels
    req->showIdx = 0;
    req->nbPlace = 0;

    int parsed = sscanf(buffer, "%d %d %d", 
                        (int *)&req->rqType, 
                        &req->showIdx, 
                        &req->nbPlace);

    if (parsed < 1) return -1;  // échec du parsing

    // Vérifier la cohérence selon le type
    switch (req->rqType) {
        case 0:  // INIT — pas de champs supplémentaires
        case 3:  // END
            return 0;
        case 1:  // CHECK — showIdx requis
            if (parsed < 2) return -1;
            return 0;
        case 2:  // BOOK — showIdx et nbPlace requis
            if (parsed < 3) return -1;
            return 0;
        default:
            return -1;  // type inconnu
    }
}


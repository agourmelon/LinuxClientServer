#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "message.h"


static void runCheckingWorker(int showIdx, char *buffer, size_t size) {

    int nbPlace = checkShowSync(showIdx);

    if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
        // Préparation du message d'erreur
        snprintf(buffer, size,
            "%d Invalid index %d, index should be between 1 and %lu.\n",
            ERROR, showIdx, NB_SHOWS);
        printf("Show index error, sending error message to client.\n");
    }
    else { // Si spectacle existe
        // Préparation du message avec nombre de places.
        snprintf(buffer, size, "%d %d", SUCCESS, nbPlace);
        printf("Checking request succeeded!\n");
    }
    // On récupère le pid du client de sa requête pour indiquer le destinataire
    response.mtype = requestPtr->clientPid;
    // Envoie du message
    if (msgsnd(msqid, &response, responseSize, 0) == -1) {
        fprintf(stderr, "msgsnd in checking service\n");
        exit(1);
    }

    printf("Worker %lu finished his job\n", pthread_self());
    return NULL;
}


static void *runBookingWorker(void* req){
    Response response;
    Request *requestPtr = (Request *)req;

    printf("Worker %lu: handle booking request of client %d.\n", pthread_self(), requestPtr->clientPid);

    // Appel à la fonction de réservation.
    // On récupère le pid du client de sa requête pour indiquer le destinataire
    response.mtype = requestPtr->clientPid;
    // Envoie du message
    if (msgsnd(msqid, &response, responseSize, 0) == -1) {
        fprintf(stderr, "msgsnd in booking service\n");
        exit(1);
    }
    
    printf("Worker %lu finished his job\n", pthread_self());
    return NULL;
}

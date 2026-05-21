#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include "message.h"
#include "sync.h"
#include "booking.h"

// descripteur de la file de message.
static int msqid;

// Tampon global pour l'affichage de la liste des spectacles.
static char showIdxMap[1024];

static void *runInitWorker(void* req) {
    Response response;
    Request *requestPtr = (Request *)req;

    printf("Worker %lu: handle init request of client %d.\n", pthread_self(), requestPtr->clientPid);

    // Nettoyage du buffer de réponse
    memset(response.message, 0, sizeof(response.message));

    // Préparation message avec liste spectacles.
    snprintf(response.message, sizeof(response.message), "%s", showIdxMap);
    response.rtype = SUCCESS;

    // On récupère le pid du client de sa requête pour indiquer le destinataire
    response.mtype = requestPtr->clientPid;

    // Envoie du message
    if (msgsnd(msqid, &response, responseSize, 0) == -1) {
        fprintf(stderr, "msgsnd in init service\n");
        exit(1);
    }

    printf("Worker %lu finished his job\n", pthread_self());
    return NULL;
}


static void *runCheckingWorker(void* req) {
    Response response;
    Request *requestPtr = (Request *)req;

    printf("Worker %lu: handle checking request of client %d.\n", pthread_self(), requestPtr->clientPid);

    // Récupération du nombre de place.
    int nbPlace = checkShowSync(requestPtr->showIdx);

    // Nettoyage du buffer de réponse
    memset(response.message, 0, sizeof(response.message));

    if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
        // Préparation du message d'erreur
        snprintf(response.message, sizeof(response.message),
            "ERROR: Invalid index %d, index should be between 1 and %lu.\n", requestPtr->showIdx, NB_SHOWS);
        response.rtype = ERROR;
        printf("Show index error, sending error message to client.\n");
    }
    else { // Si spectacle existe
        // Préparation du message avec nombre de places.
        snprintf(response.message, sizeof(response.message), "%d", nbPlace);
        response.rtype = SUCCESS;
        printf("Request of Client %d succeeded!\n", requestPtr->clientPid);
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
    int result = bookShowSync(requestPtr->showIdx, requestPtr->nbPlace);

    // Nettoyage du buffer de réponse
    memset(response.message, 0, sizeof(response.message));
    if (result==-1) { // Si indice du spectacle n'existe pas:
        // Préparation du message d'erreur
        snprintf(response.message, sizeof(response.message),
            "ERROR: Invalid index %d, index should be between 1 and %lu.\n", requestPtr->showIdx, NB_SHOWS);
        response.rtype = ERROR;
        printf("Show index error, sending error message to client.\n");
    } else if (result==-2) { // Si pas assez de places disponibles:
        // Préparation du message d'erreur
        snprintf(response.message, sizeof(response.message),
            "ERROR: Not enough remaining places. Remaining: %d, requested: %d.\n",
            checkShowSync(requestPtr->showIdx), requestPtr->nbPlace);
        response.rtype = ERROR;
        printf("Not enough places error, sending error message to client.\n");
    } else { // Le spectacle existe et le nombre de places est suffisant
        // Préparation du message de confirmation.
        snprintf(response.message, sizeof(response.message),
            "%d places booked for %s", result, SHOWS[requestPtr->showIdx-1].title);
        response.rtype = SUCCESS;
        printf("Request of Client %d succeeded!\n", requestPtr->clientPid);
    }
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

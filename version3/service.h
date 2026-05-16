#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include "message.h"
#include "sync.h"
#define INIT_MTYPE 1
#define CHECK_MTYPE 2
#define BOOK_MTYPE 3

// descripteur de la file de message.
static int msqid;

// Tampon global pour l'affichage de la liste des spectacles.
static char showIdxMap[1024];

static void runInitService() {
    Response response;
    Request request;

    printf("Launching Init service...\n");

    // Impression de la liste des spectacle dans le buffer
    printShowIndexMap(showIdxMap, 1024);
    while(1){
        // Attente d'un messages.
        if (msgrcv(msqid, &request, requestSize, INIT_MTYPE, 0) == -1) {
            fprintf(stderr, "msgrcv in init service\n");
            exit(1);
        } 

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        printf("Client %d request to see list of shows.\n", request.clientPid);
        // Préparation message avec liste spectacles.
        snprintf(response.message, sizeof(response.message), "%s", showIdxMap);
        response.rtype = SUCCESS;

        // On récupère le pid du client de sa requête pour indiquer le destinataire
        response.mtype = request.clientPid;

        // Envoie du message
        if (msgsnd(msqid, &response, responseSize, 0) == -1) {
            fprintf(stderr, "msgsnd in init service\n");
            exit(1);
        }
    }
}


static void runCheckingService(){
    Response response;
    Request request;

    printf("Launching checking service...\n");
    while(1){
        // Attente d'un messages.
        if (msgrcv(msqid, &request, requestSize, CHECK_MTYPE, 0) == -1) { 
            fprintf(stderr, "msgrcv in checking service\n");
            exit(1);
        }
        printf("Client %d request to see remaining place for a show.\n", request.clientPid);

        // Récupération du nombre de place.
        int nbPlace = checkShowSync(request.showIdx);

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
            // Préparation du message d'erreur
            snprintf(response.message, sizeof(response.message),
                "ERROR: Invalid index %d, index should be between 1 and %lu.\n", request.showIdx, NB_SHOWS);
            response.rtype = ERROR;
            printf("Show index error, sending error message to client.\n");
        }
        else { // Si spectacle existe
            // Préparation du message avec nombre de places.
            snprintf(response.message, sizeof(response.message), "%d", nbPlace);
            response.rtype = SUCCESS;
            printf("Request of Client %d succeeded!\n", request.clientPid);
        }
        // On récupère le pid du client de sa requête pour indiquer le destinataire
        response.mtype = request.clientPid;
        // Envoie du message
        if (msgsnd(msqid, &response, responseSize, 0) == -1) {
            fprintf(stderr, "msgsnd in checking service\n");
            exit(1);
        }
    }
}


static void runBookingService(){
    Response response;
    Request request;

    printf("Launching booking service...\n");
    while(1){
        // Attente d'un messages.
        if (msgrcv(msqid, &request, requestSize, BOOK_MTYPE, 0) == -1) { 
            fprintf(stderr, "msgrcv in booking service\n");
            exit(1);
        } 
        printf("Client %d request to book places for a show.\n", request.clientPid);

        // Appel à la fonction de réservation.
        int result = bookShowSync(request.showIdx, request.nbPlace);

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));
        if (result==-1) { // Si indice du spectacle n'existe pas:
            // Préparation du message d'erreur
            snprintf(response.message, sizeof(response.message),
                "ERROR: Invalid index %d, index should be between 1 and %lu.\n", request.showIdx, NB_SHOWS);
            response.rtype = ERROR;
            printf("Show index error, sending error message to client.\n");
        } else if (result==-2) { // Si pas assez de places disponibles:
            // Préparation du message d'erreur
            snprintf(response.message, sizeof(response.message),
                "ERROR: Not enough remaining places. Remaining: %d, requested: %d.\n",
                checkShowSync(request.showIdx), request.nbPlace);
            response.rtype = ERROR;
            printf("Not enough places error, sending error message to client.\n");
        } else { // Le spectacle existe et le nombre de places est suffisant
            // Préparation du message de confirmation.
            snprintf(response.message, sizeof(response.message),
                "%d places booked for %s", result, SHOWS[request.showIdx-1].title);
            response.rtype = SUCCESS;
            printf("Request of Client %d succeeded!\n", request.clientPid);
        }
        // On récupère le pid du client de sa requête pour indiquer le destinataire
        response.mtype = request.clientPid;
        // Envoie du message
        if (msgsnd(msqid, &response, responseSize, 0) == -1) {
            fprintf(stderr, "msgsnd in booking service\n");
            exit(1);
        }
    }
}

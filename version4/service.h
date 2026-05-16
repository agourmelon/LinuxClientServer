#pragma once

#include <string.h>
#include "worker.h"
#define INIT_MTYPE 1
#define CHECK_MTYPE 2
#define BOOK_MTYPE 3


static void *runInitService(void* args) {
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

        printf("Client %d request to see list of shows.\n", request.clientPid);

        // Traitement de la tâche dans un worker séparé
        pthread_t worker;
        pthread_create(&worker, NULL, runInitWorker, &request);

        // Le worker continue sa tache en asynchrone.
        pthread_detach(worker);
    }
}



static void *runCheckingService(void* args){
    Request request;

    printf("Launching checking service...\n");
    while(1){
        // Attente d'un messages.
        if (msgrcv(msqid, &request, requestSize, CHECK_MTYPE, 0) == -1) { 
            fprintf(stderr, "msgrcv in checking service\n");
            exit(1);
        }

        printf("Client %d request to see remaining place for a show.\n", request.clientPid);
        
        // Traitement de la tâche dans un worker séparé
        pthread_t worker;
        pthread_create(&worker, NULL, runCheckingWorker, &request);

        // Le worker continue sa tache en asynchrone.
        pthread_detach(worker);
    }
}


static void *runBookingService(void* args){
    Request request;

    printf("Launching booking service...\n");
    while(1){
        // Attente d'un messages.
        if (msgrcv(msqid, &request, requestSize, BOOK_MTYPE, 0) == -1) { 
            fprintf(stderr, "msgrcv in booking service\n");
            exit(1);
        } 

        printf("Client %d request to book places for a show.\n", request.clientPid);

        // Traitement de la tâche dans un worker séparé
        pthread_t worker;
        pthread_create(&worker, NULL, runBookingWorker, &request);

        // Le worker continue sa tache en asynchrone.
        pthread_detach(worker);
    }
}


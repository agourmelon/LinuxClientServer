#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/sem.h>
#include "message.h"
#include "../booking.h"
#define MSQKEY 17
#define SEMKEY1 18
#define SEMKEY2 19
#define INIT_MTYPE 1
#define CHECK_MTYPE 2
#define BOOK_MTYPE 3

// descripteur de la file de message.
int msqid;
// descripteurs des mutexes (semaphores) pour la synchronisation 
// sur le tableau des spectacles: architecture lecteurs/écrivains.
int mutexNbCheckersId, mutexBookId;

// Nombre de lecteurs.
int nbCheckers;

// Operations sur semaphore d'indice showIdx de l'ensemble semid.
void semopV(int semid, int showIdx); // Operation V
void SemopP(int semid, int showIdx); // Operation P

// Signal handler pour sigint
void sigintHandler(int _);

// Fonctions associés aux différent services
void runInitService(); // initialisation: affichage listes spectacles
void runCheckingService(); // consultation du nombre de places
void runBookingService(); // réservation de places pour un spectacle


int main() {
    // Initialisation des variables
    pthread_t threadInit, threadCheck, threadBook;
    mode_t ipcFlag = IPC_CREAT | IPC_EXCL | 0666;
    
    // Message de lancement du server.
    printf("Booking server is launched...\n");

    // Céation de la file de messages.
    msqid = msgget(MSQKEY, ipcFlag);
    printf("Message queue created!\n");

    // Création des mutexes: 1 de chaques par entrée du tableau (NB_SHOWS)
    mutexNbCheckersId = semget(SEMKEY1, NB_SHOWS, ipcFlag);
    mutexBookId = semget(SEMKEY2, NB_SHOWS, ipcFlag);
    // Initialisation des mutexes (max nb_token = 1)
    semctl(mutexNbCheckersId, 0, SETALL, 1);
    semctl(mutexBookId, 0, SETALL, 1);

    // On redéfinit l'action du sigint: détruire proprement la file de message
    signal(SIGINT, sigintHandler);

    // Lancement des différents services.
    pthread_create(&threadInit, NULL, (void *(*)(void *))runInitService, NULL);
    pthread_create(&threadCheck, NULL, (void *(*)(void *))runCheckingService, NULL);
    pthread_create(&threadBook, NULL, (void *(*)(void *))runBookingService, NULL);

    // Synchronisation à l'arrêt des services
    pthread_join(threadInit, NULL);
    pthread_join(threadCheck, NULL);
    pthread_join(threadBook, NULL);

    //Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);

    return 0;
}


void sigintHandler(int _){
    // Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);
    // Destruction des semaphores/mutexes.
    semctl(mutexBookId, 0, IPC_RMID, 0);
    semctl(mutexNbCheckersId, 0, IPC_RMID, 0);
    exit(0);
}

void semopP(int semid, int showIdx) {
    struct sembuf operation;
    operation.sem_num = showIdx - 1;
    operation.sem_op = -1;
    operation.sem_flg = 0;
    semop(semid, &operation, 1);
}

void semopV(int semid, int showIdx) {
    struct sembuf operation;
    operation.sem_num = showIdx - 1;
    operation.sem_op = 1;
    operation.sem_flg = 0;
    semop(semid, &operation, 1);
}


int checkShowSync(int showIdx) {
    semopP(mutexNbCheckersId, showIdx);
    nbCheckers++;
    if (nbCheckers == 1) semopP(mutexBookId, showIdx);
    semopV(mutexNbCheckersId, showIdx);
    int nbPlace = checkShow(showIdx);
    semopP(mutexNbCheckersId, showIdx);
    nbCheckers--;
    if (nbCheckers == 0) semopV(mutexBookId, showIdx);
    semopV(mutexNbCheckersId, showIdx);
    return nbPlace;
}

int bookShowSync(int showIdx, int nbPlace) {
    semopP(mutexBookId, showIdx);
    int result = bookShow(showIdx, nbPlace);
    semopV(mutexBookId, showIdx);
    return result;
}


void runInitService() {
    Response response;
    Request request;
    char showIdxMap[1024];

    printf("Launching Init service...\n");

    // Impression de la liste des spectacle dans le buffer
    printShowIndexMap(showIdxMap, 1024);
    while(1){
        // Attente d'un messages.
        msgrcv(msqid, &request, requestSize, INIT_MTYPE, 0); 

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        printf("Client %d request to see list of shows.\n", request.clientPid);
        // Préparation message avec liste spectacles.
        snprintf(response.message, sizeof(response.message), "%s", showIdxMap);
        response.rtype = SUCCESS;

        // On récupère le pid du client de sa requête pour indiquer le destinataire
        response.mtype = request.clientPid;
        // Envoie du message
        msgsnd(msqid, &response, responseSize, 0);
    }
}


void runCheckingService(){
    Response response;
    Request request;

    printf("Launching checking service...\n");
    while(1){
        // Attente d'un messages.
        msgrcv(msqid, &request, requestSize, CHECK_MTYPE, 0); 
        printf("Client %d request to see remaining place for a show.\n", request.clientPid);

        // Récupération du nombre de place.
        int nbPlace = checkShowSync(request.showIdx);

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
            // Préparation du message d'erreur
            snprintf(response.message, sizeof(response.message),
                "ERROR: Invalid index %d, index should be between 1 and %d.\n", request.showIdx, (int)NB_SHOWS);
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
        msgsnd(msqid, &response, responseSize, 0);
    }
}


void runBookingService(){
    Response response;
    Request request;

    printf("Launching booking service...\n");
    while(1){
        // Attente d'un messages.
        msgrcv(msqid, &request, requestSize, BOOK_MTYPE, 0); 
        printf("Client %d request to book places for a show.\n", request.clientPid);

        // Appel à la fonction de réservation.
        int result = bookShowSync(request.showIdx, request.nbPlace);

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));
        if (result==-1) { // Si indice du spectacle n'existe pas:
            // Préparation du message d'erreur
            snprintf(response.message, sizeof(response.message),
                "ERROR: Invalid index %d, index should be between 1 and %d.\n", request.showIdx, (int)NB_SHOWS);
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
        msgsnd(msqid, &response, responseSize, 0);
    }
}

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
#define NB_SHOWS sizeof(SHOWS) / sizeof(ShowBookingInfo) // Forcer NB_SHOWS comme valeur constante (sinon erreur compilation l.15)
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
int nbCheckers[NB_SHOWS];
    
// Tampon global pour l'affichage de la liste des spectacles.
static char showIdxMap[1024];

// Operations sur semaphore d'indice showIdx de l'ensemble semid.
void semopV(int semid, int showIdx); // Operation V
void SemopP(int semid, int showIdx); // Operation P

// Signal handler pour sigint
void sigintHandler(int _);

// Fonctions associés aux différent services
void *runInitService(void* arg); // initialisation: affichage listes spectacles
void *runCheckingService(void* arg); // consultation du nombre de places
void *runBookingService(void* arg); // réservation de places pour un spectacle

// Fonctions des workers pour chacun des services
void *runInitWorker(void* req); // initialisation: affichage listes spectacles
void *runCheckingWorker(void* req); // consultation du nombre de places
void *runBookingWorker(void* req); // réservation de places pour un spectacle

// flag pour la création des IPCs.
mode_t ipcFlag = IPC_CREAT | IPC_EXCL | 0666;


///////////////////////////////////////////////////////////////////////////////////
//                                     MAIN                                      //
///////////////////////////////////////////////////////////////////////////////////

int main() {
    // Descripteur des threads de services
    pthread_t threadInit, threadCheck, threadBook;
    
    // initialisation des nombres de lecteurs à 0.
    memset(nbCheckers, 0, NB_SHOWS * sizeof(int));
    
    // Message de lancement du server.
    printf("Booking server is launched...\n");

    // Céation de la file de messages.
    msqid = msgget(MSQKEY, ipcFlag);
    printf("Message queue created!\n");

    // Création des mutexes: 1 de chaques par entrée du tableau (NB_SHOWS)
    mutexNbCheckersId = semget(SEMKEY1, NB_SHOWS, ipcFlag);
    mutexBookId = semget(SEMKEY2, NB_SHOWS, ipcFlag);

    // Initialisation des mutexes (max nb_token = 1)
    unsigned short vals[NB_SHOWS];
    for (long unsigned int i = 0; i < NB_SHOWS; i++) vals[i] = 1;
    semctl(mutexNbCheckersId, 0, SETALL, vals);
    semctl(mutexBookId, 0, SETALL, vals);

    // On redéfinit l'action du sigint: détruire proprement la file de message
    signal(SIGINT, sigintHandler);

    // Lancement des différents services.
    pthread_create(&threadInit, NULL, runInitService, NULL);
    pthread_create(&threadCheck, NULL, runCheckingService, NULL);
    pthread_create(&threadBook, NULL, runBookingService, NULL);

    // Synchronisation à l'arrêt des services
    pthread_join(threadInit, NULL);
    pthread_join(threadCheck, NULL);
    pthread_join(threadBook, NULL);

    // Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//                                    SIGNALS                                    //
///////////////////////////////////////////////////////////////////////////////////

void sigintHandler(int _){
    // Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);
    // Destruction des semaphores/mutexes.
    semctl(mutexBookId, 0, IPC_RMID, 0);
    semctl(mutexNbCheckersId, 0, IPC_RMID, 0);
    exit(0);
}


///////////////////////////////////////////////////////////////////////////////////
//                               SYNCHRONISATION                                 //
///////////////////////////////////////////////////////////////////////////////////

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
    // Validation de showIdx
    if (showIdx <=0 || (long unsigned int)showIdx > NB_SHOWS) return -1;

    semopP(mutexNbCheckersId, showIdx);
    nbCheckers[showIdx-1]++;
    if (nbCheckers[showIdx-1] == 1) semopP(mutexBookId, showIdx);
    semopV(mutexNbCheckersId, showIdx);

    int nbPlace = checkShow(showIdx);

    semopP(mutexNbCheckersId, showIdx);
    nbCheckers[showIdx-1]--;
    if (nbCheckers[showIdx-1] == 0) semopV(mutexBookId, showIdx);
    semopV(mutexNbCheckersId, showIdx);
    return nbPlace;
}

int bookShowSync(int showIdx, int nbPlace) {
    // Validation de showIdx
    if (showIdx <=0 || (long unsigned int)showIdx > NB_SHOWS) return -1;

    semopP(mutexBookId, showIdx);
    int result = bookShow(showIdx, nbPlace);
    semopV(mutexBookId, showIdx);
    return result;
}


///////////////////////////////////////////////////////////////////////////////////
//                                   SERVICES                                    //
///////////////////////////////////////////////////////////////////////////////////

void *runInitService(void* args) {
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



void *runCheckingService(void* args){
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


void *runBookingService(void* args){
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


///////////////////////////////////////////////////////////////////////////////////
//                                    WORKERS                                    //
///////////////////////////////////////////////////////////////////////////////////

void *runInitWorker(void* req) {
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


void *runCheckingWorker(void* req) {
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


void *runBookingWorker(void* req){
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
            "ERROR: Not enough remaining places. Remaining: %d, requestPtr->d: %d.\n",
            checkShow(requestPtr->showIdx), requestPtr->nbPlace);
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


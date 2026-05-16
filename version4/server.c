#include <signal.h>
#include "service.h"
#define MSQKEY 17

// Signal handler pour sigint
void sigintHandler(int _);

// flag pour la création des semaphores/mutexes.
mode_t msqFlag = IPC_CREAT | IPC_EXCL | 0666;

int main() {
    // Descripteur des threads de services
    pthread_t threadInit, threadCheck, threadBook;
    
    // Message de lancement du server.
    printf("Booking server is launched...\n");

    // Céation de la file de messages.
    msqid = msgget(MSQKEY, msqFlag);
    printf("Message queue created!\n");

    // On redéfinit l'action du sigint: détruire proprement la file de message
    signal(SIGINT, sigintHandler);

    // Initialisation des structures de synchronisation
    initializeSyncStructures();

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


void sigintHandler(int _){
    // Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);
    // Destruction des semaphores/mutexes.
    semctl(mutexBookId, 0, IPC_RMID, 0);
    semctl(mutexNbCheckersId, 0, IPC_RMID, 0);
    exit(0);
}


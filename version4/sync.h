#pragma once

#include <string.h>
#include <sys/sem.h>
#include "../booking.h"
#define NB_SHOWS sizeof(SHOWS) / sizeof(ShowBookingInfo) // Forcer NB_SHOWS comme valeur constante (sinon erreur compilation l.30)
#define SEMKEY1 18
#define SEMKEY2 19

// descripteurs des mutexes (semaphores) pour la synchronisation 
// sur le tableau des spectacles: architecture lecteurs/écrivains.
static int mutexNbCheckersId, mutexBookId;

// Nombre de lecteurs.
static int nbCheckers[NB_SHOWS];

// flag pour la création des semaphores/mutexes.
static mode_t semFlag = IPC_CREAT | IPC_EXCL | 0666;


static void initializeSyncStructures(){
    // initialisation des nombres de lecteurs à 0.
    memset(nbCheckers, 0, NB_SHOWS * sizeof(int));

    // Création des mutexes: 1 de chaques par entrée du tableau (NB_SHOWS)
    mutexNbCheckersId = semget(SEMKEY1, NB_SHOWS, semFlag);
    mutexBookId = semget(SEMKEY2, NB_SHOWS, semFlag);

    // Initialisation des mutexes (max nb_token = 1)
    unsigned short vals[NB_SHOWS];
    for (long unsigned int i = 0; i < NB_SHOWS; i++) vals[i] = 1;
    semctl(mutexNbCheckersId, 0, SETALL, vals);
    semctl(mutexBookId, 0, SETALL, vals);
}
    
static void semopP(int semid, int showIdx) {
    struct sembuf operation;
    operation.sem_num = showIdx - 1;
    operation.sem_op = -1;
    operation.sem_flg = 0;
    semop(semid, &operation, 1);
}

static void semopV(int semid, int showIdx) {
    struct sembuf operation;
    operation.sem_num = showIdx - 1;
    operation.sem_op = 1;
    operation.sem_flg = 0;
    semop(semid, &operation, 1);
}


static int checkShowSync(int showIdx) {
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

static int bookShowSync(int showIdx, int nbPlace) {
    // Validation de showIdx
    if (showIdx <=0 || (long unsigned int)showIdx > NB_SHOWS) return -1;

    semopP(mutexBookId, showIdx);
    int result = bookShow(showIdx, nbPlace);
    semopV(mutexBookId, showIdx);
    return result;
}


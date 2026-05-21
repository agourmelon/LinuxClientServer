#include <signal.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "service.h"
#include "sync.h"
#define SHMKEY     256
#define SHMKEY_NBR 257
#define PORTS "2058"
#define MAXCLIENT 10

// Signal handler pour sigint
void sigintHandler(int _);

// Fonction utilitaire lors de l'arrêt du programme
void safeCloseServer();

// flag pour la création des semaphores/mutexes.
mode_t msqFlag = IPC_CREAT | IPC_EXCL | 0666;

// Préparation du segment de mémoire partagé
ShowBookingInfo *initShowsMemory();
int             *initNbCheckersMemory();

ShowBookingInfo *sharedShows;
int             *sharedNbCheckers;

pid_t parentPid;

int shmid;
int shmidNbr;

int listenSockFd, serviceSockFd;

int main() {
    struct addrinfo hints, *serverInfo;
    struct sockaddr clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    pid_t newPid;

    sharedShows      = initShowsMemory();
    sharedNbCheckers = initNbCheckersMemory();

    initializeSyncStructures();
    setSyncPointers(sharedShows, sharedNbCheckers);

    parentPid = getpid();

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, PORTS, &hints, &serverInfo) == -1) {
        fprintf(stderr, "GETADDRINFO FAILURE \n");
        exit(1);
    }

    listenSockFd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (listenSockFd == -1) {
        fprintf(stderr, "LISTENER SOCKET CREATION FAILURE \n");
        exit(1);
    }
    if (bind(listenSockFd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
        fprintf(stderr, "LISTENER SOCKET BINDING FAILURE \n");
        exit(1);
    }
    freeaddrinfo(serverInfo);
    if (listen(listenSockFd, MAXCLIENT) == -1){
        fprintf(stderr, "LISTENER SOCKET LISTEN FAILURE \n");
        exit(1);
    }
    printf("Socket %d ready to listen \n", listenSockFd);

    signal(SIGINT, sigintHandler);
    signal(SIGCHLD, SIG_IGN); // évite les processus zombies sans avoir à faire wait() sur chaque enfant

    printf("Booking server is launched...\n");

    printShowIndexMap(sharedShows, showIdxMap, showIdxMapSize);

    while (1) {
        serviceSockFd = accept(listenSockFd, &clientAddr, &clientAddrSize);
        if (serviceSockFd == -1) {
            fprintf(stderr, "SERVICE SOCKET ACCEPT FAILURE \n");
            exit(1);
        }
        printf("New request received from a Client at address %s.\n", clientAddr.sa_data);
        newPid = fork();
        if (newPid == 0) {
            close(listenSockFd);
            runService(serviceSockFd, &clientAddr);
            shmdt(sharedShows);
            shmdt(sharedNbCheckers);
            close(serviceSockFd);
            exit(0);
        }
        else {
            printf("Client at address %s, handle by worker %d.\n", clientAddr.sa_data, newPid);
            close(serviceSockFd);
        }
    }

    safeCloseServer();
    return 0;
}

ShowBookingInfo *initShowsMemory() {
    shmid = shmget(SHMKEY, NB_SHOWS * sizeof(ShowBookingInfo), IPC_CREAT | 0666);
    ShowBookingInfo *shows = shmat(shmid, NULL, 0);
    memcpy(shows, SHOWS_INIT, NB_SHOWS * sizeof(ShowBookingInfo));
    return shows;
}

int *initNbCheckersMemory() {
    shmidNbr = shmget(SHMKEY_NBR, NB_SHOWS * sizeof(int), IPC_CREAT | 0666);
    int *nbCheckers = shmat(shmidNbr, NULL, 0);
    memset(nbCheckers, 0, NB_SHOWS * sizeof(int));
    return nbCheckers;
}

void safeCloseServer() {
    close(listenSockFd);
    shmdt(sharedShows);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(sharedNbCheckers);
    shmctl(shmidNbr, IPC_RMID, NULL);
    semctl(mutexBookId, 0, IPC_RMID, 0);
    semctl(mutexNbCheckersId, 0, IPC_RMID, 0);
}

void sigintHandler(int _) {
    if (getpid() == parentPid) { // seul le parent détruit les ressources IPC partagées; les enfants se contentent de se détacher
        safeCloseServer();
    } else {
        close(serviceSockFd);
        shmdt(sharedShows);
        shmdt(sharedNbCheckers);
    }
    exit(0);
}

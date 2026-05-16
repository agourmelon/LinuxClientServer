#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "message.h"
#define MSQKEY 17
#define INIT_MTYPE 1
#define CHECK_MTYPE 2
#define BOOK_MTYPE 3

int main() {
    // Initialisation des variables
    int msqid;
    int showIdx, nbPlace;
    Request request;
    Response response;
    char showIdxMap[1024];
    int requestNb;
    pid_t pid = getpid();

    printf("Client %d is launched...\n", pid);

    // Récupération de la file de message
    msqid = msgget(MSQKEY, 0);

    // Requête initiale: récupération de la liste des spectacles
        // Préparation du message
    request.mtype = INIT_MTYPE;
    request.clientPid = pid;
        // On ignore les champs showIdx et nbPlace: inutile dans cette requête
        // Envoie de la requête
    msgsnd(msqid, &request, requestSize, 0);
    printf("Requesting list of shows.\n");
        // Nettoyage du buffer de réponse
    memset(response.message, 0, sizeof(response.message));
        // Attente de la réponse
    msgrcv(msqid, &response, responseSize, pid, 0);
        // Sauvegarde de la liste dans la variable local showIdxMap
    snprintf(showIdxMap, sizeof(showIdxMap), "%s", response.message);

    while (1) {
        printf("Here is the list of available shows.\n");
        printf("%s\n", showIdxMap);

        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        // Demande de l'action souhaitée
        printf("What is your request?\n 1. Get remaining places for a show.\n 2. Book places for a show.\n --> ");
        fflush(stdout);
        scanf("%d", &requestNb);

        // Requête de consultation
        if (requestNb == 1) { 

            // Choix du spectacle (via son indice)
            printf("Enter show index to see remaining places: ");
            fflush(stdout);
            scanf("%d", &showIdx);

            // Préparation de la requête (Champs nécessaires)
            request.mtype = CHECK_MTYPE;
            request.showIdx = showIdx;
            request.clientPid = pid;

            // Envoie de la requête
            msgsnd(msqid, &request, requestSize, 0);

            // Réception et affichage de la réponse
            msgrcv(msqid, &response, responseSize, pid, 0);
            if (response.rtype == SUCCESS)
            printf("Remaining places: %s\n\n", response.message);
            else printf("%s\n", response.message);

        } else if (requestNb == 2) {

            // Choix du spectacle et du nombre de places
            printf("Enter show index and number of places to book (ex: 1 5): ");
            fflush(stdout);
            scanf("%d %d", &showIdx, &nbPlace);

            // Préparation de la requête
            request.mtype = BOOK_MTYPE;
            request.showIdx = showIdx;
            request.nbPlace = nbPlace;
            request.clientPid = pid;

            // Envoie de la requête
            msgsnd(msqid, &request, requestSize, 0);

            // Réception et affichage de la réponse
            msgrcv(msqid, &response, responseSize, pid, 0);
            if (response.rtype == SUCCESS)
            printf("Server response: %s\n\n", response.message);
            else printf("%s\n", response.message);

        } else {
            printf("Unknown request: %d\n\n", requestNb);
        }

    }

    return 0;
}

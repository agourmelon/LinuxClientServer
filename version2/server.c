#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include "message.h"
#define MSQKEY 17

typedef struct {
    const char* title;
    int remainingPlace;
} ShowBookingInfo;

ShowBookingInfo BOOKABLE_SHOWS[] = {
    {"Mamma Mia", 100},
    {"The Cid", 100},
    {"West Side Story", 100},
    {"Hamlett", 100}
};

const size_t NB_SHOWS = sizeof(BOOKABLE_SHOWS) / sizeof(ShowBookingInfo);

int bookShow(size_t index, unsigned int nbPlace) {
    index--;
    if (index >= NB_SHOWS) return -1;
    if (nbPlace > (unsigned int)BOOKABLE_SHOWS[index].remainingPlace) return -2;
    BOOKABLE_SHOWS[index].remainingPlace -= nbPlace;
    return nbPlace;
}

int getShowRemainingPlaces(size_t index) {
    index--;
    if (index >= NB_SHOWS) return -1;
    return BOOKABLE_SHOWS[index].remainingPlace;
}

int printShowIndexMap(char *buffer, size_t bufferSize) {
    int position = 0;
    for (int i = 0; i < (int)NB_SHOWS; i++) {
        ShowBookingInfo show = BOOKABLE_SHOWS[i];
        position += snprintf(
            buffer + position,
            bufferSize - position,
            "%d. %-20s\n",
            i + 1, show.title);
        if (position >= (int)bufferSize) return -1;
    }
    return position;
}

int main() {
    // Initialisation des variables
    int msqid;
    int showIdx, nbPlace;
    Request request;
    Response response;
    mode_t ipcFlag = IPC_CREAT | IPC_EXCL | 0666;
    char showIdxMap[1024];
    
    // Message de lancement du server.
    printf("Booking server is launched...\n");

    // Céation de la file de messages.
    msqid = msgget(MSQKEY, ipcFlag);
    printf("Message queue created!");

    // Impression de la liste des spectacle dans le buffer
    printShowIndexMap(showIdxMap, 1024);

    // Boucle infine.
    while(1){
        
        // Attente d'un messages.
        printf("Waiting for client request...\n");
        msgrcv(msqid, &request, requestSize, 1, 0); // Ici on utilise pas le multiplexage: mtype=1 toujours.
                                                    //
        // Nettoyage du buffer de réponse
        memset(response.message, 0, sizeof(response.message));

        // Si requete init:
        if (request.rtype == INIT) {
            // Préparation message avec liste spectacles.
            snprintf(response.message, sizeof(response.message), "%s", showIdxMap);
        }
        // Si requête de consultation:
        else if (request.rtype == CHECK) {
            printf("Client %d request to see remaining place for a show.\n", request.clientPid);
            // Récupération du nombre de place.
            nbPlace = getShowRemainingPlaces(request.showIdx);
            if (nbPlace==-1) { // Si spectacle n'existe pas (indice trop grand):
                // Préparation du message d'erreur
                snprintf(response.message, sizeof(response.message),
                    "ERROR: Invalid index %d, maximum index is %d.", request.showIdx, (int)NB_SHOWS);
                printf("Show index error, sending error message to client.\n");
            }
            else { // Si spectacle existe
                // Préparation du message avec nombre de places.
                snprintf(response.message, sizeof(response.message), "%d", nbPlace);
                printf("Request of Client %d succeeded!\n", request.clientPid);
            }
        }
        // Si requête de réservation.
        else if (request.rtype == BOOK) {
            printf("Client %d request to book places for a show.\n", request.clientPid);
            // Appel à la fonction de réservation.
            int result = bookShow(request.showIdx, request.nbPlace);
            if (result==-1) { // Si indice du spectacle n'existe pas:
                // Préparation du message d'erreur
                snprintf(response.message, sizeof(response.message),
                    "ERROR: Invalid index %d, maximum index is %d.", request.showIdx, (int)NB_SHOWS);
                printf("Show index error, sending error message to client.\n");
            } else if (result==-2) { // Si pas assez de places disponibles:
                // Préparation du message d'erreur
                snprintf(response.message, sizeof(response.message),
                    "ERROR: Not enough remaining places. Remaining: %d, requested: %d.",
                    getShowRemainingPlaces(showIdx), nbPlace);
                printf("Not enough places error, sending error message to client.\n");
            } else { // Le spectacle existe et le nombre de places est suffisant
                // Préparation du message de confirmation.
                snprintf(response.message, sizeof(response.message),
                    "%d places booked for %s", result, BOOKABLE_SHOWS[showIdx].title);
                printf("Request of Client %d succeeded!\n", request.clientPid);
            }
        }
        // Si requête invalide
        else {
            printf("Unknown request: %d from Client %d \n", request.rtype, request.clientPid);
            snprintf(response.message, sizeof(response.message), "ERROR: Unknown request.");
        }
        // On récupère le pid du client de sa requête pour indiquer le destinataire
        response.mtype = request.clientPid;
        // Envoie du message
        msgsnd(msqid, &response, ResponseSize, 0);
    }

    //Destruction de la file de message.
    msgctl(msqid, IPC_RMID, NULL);

    return 0;
}

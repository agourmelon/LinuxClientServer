#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "booking.h"

void sigintHandler(int _) {
    unlink("client2server");
    unlink("server2client");
    exit(0);
}

int main() {
    char msgBuffer[1024];
    char showIdxMap[NB_SHOWS * 64];
    int fromClientTubeFd, toClientTubeFd;
    mode_t mode = S_IRUSR | S_IWUSR;
    int showIdx;
    int nbPlace;

    printf("Booking server is launched...\n");

    mkfifo("client2server", mode);
    mkfifo("server2client", mode);

    signal(SIGINT, sigintHandler);
    signal(SIGPIPE, SIG_IGN); // évite le crash du serveur si le client se déconnecte en cours d'écriture

    // open() sur un FIFO bloque jusqu'à ce que l'autre extrémité soit ouverte.
    // Le serveur et le client ouvrent les FIFOs dans le même ordre (client2server puis server2client)
    // mais avec des modes opposés (lecture/écriture) — même ordre obligatoire pour éviter l'interblocage.
    printf("Opening client to server channel\n");
    fromClientTubeFd = open("client2server", O_RDONLY);
    printf("Opening server to client channel\n");
    toClientTubeFd = open("server2client", O_WRONLY);

    printShowIndexMap(showIdxMap, NB_SHOWS * 64);

    printf("Sending the list of shows to client.\n");
    write(toClientTubeFd, showIdxMap, NB_SHOWS * 64);

    while (1) {

        printf("Waiting for client request...\n");
        memset(msgBuffer, 0, sizeof(msgBuffer));
        read(fromClientTubeFd, msgBuffer, sizeof(msgBuffer) - 1 );

        if (strcmp(msgBuffer, "1") == 0) {
            printf("Client request to see remaining place for a show.\n");
            write(toClientTubeFd, msgBuffer, strlen(msgBuffer) + 1); // ACK : signale au client que le serveur est prêt à recevoir les paramètres

            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromClientTubeFd, msgBuffer, sizeof(msgBuffer) - 1);
            sscanf(msgBuffer, "%d", &showIdx);
            nbPlace = checkShow(showIdx);
            if (nbPlace == -1) {
                snprintf(msgBuffer, sizeof(msgBuffer),
                    "ERROR: Invalid index %d, index should be between 1 and %d.\n", showIdx, (int)NB_SHOWS);
                printf("Show index error, sending error message to client.\n");
            } else {
                snprintf(msgBuffer, sizeof(msgBuffer), "%d", nbPlace);
                printf("Request succeeded!\n");
            }

        } else if (strcmp(msgBuffer, "2") == 0) {
            printf("Client request to book places for a show.\n");
            write(toClientTubeFd, msgBuffer, strlen(msgBuffer) + 1); // ACK : signale au client que le serveur est prêt à recevoir les paramètres

            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromClientTubeFd, msgBuffer, sizeof(msgBuffer) - 1);
            sscanf(msgBuffer, "%d %d", &showIdx, &nbPlace);
            int result = bookShow(showIdx, nbPlace);
            if (result == -1) {
                snprintf(msgBuffer, sizeof(msgBuffer),
                    "ERROR: Invalid index %d, index should be between 1 and %d.\n", showIdx, (int)NB_SHOWS);
                printf("Show index error, sending error message to client.\n");
            } else if (result == -2) {
                snprintf(msgBuffer, sizeof(msgBuffer),
                    "ERROR: Not enough remaining places. Remaining: %d, requested: %d.\n",
                    checkShow(showIdx), nbPlace);
                printf("Not enough places error, sending error message to client.\n");
            } else {
                snprintf(msgBuffer, sizeof(msgBuffer),
                    "%d places booked for %s", result, SHOWS[showIdx-1].title);
                printf("Request succeeded!\n");
            }

        } else {
            printf("Unknown request: %s\n", msgBuffer);
            snprintf(msgBuffer, sizeof(msgBuffer), "ERROR: Unknown request.");
        }

        write(toClientTubeFd, msgBuffer, strlen(msgBuffer) + 1);

    }

    close(fromClientTubeFd);
    close(toClientTubeFd);

    return 0;
}

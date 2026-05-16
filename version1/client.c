#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {
    char msgBuffer[1024];
    char showIdxMap[1024];
    int fromServerTubeFd, toServerTubeFd;
    int requestNb;
    int showIdx;
    int nbPlace;

    toServerTubeFd = open("client2server", O_WRONLY);
    fromServerTubeFd = open("server2client", O_RDONLY);

    // Réception de la liste des spectacles
    memset(showIdxMap, 0, sizeof(showIdxMap));
    read(fromServerTubeFd, showIdxMap, sizeof(showIdxMap) - 1);

    while (1) {
        printf("Here is the list of available shows.\n");
        printf("%s\n", showIdxMap);

        // Demande de l'action souhaitée
        printf("What is your request?\n 1. Get remaining places for a show.\n 2. Book places for a show.\n --> ");
        fflush(stdout);
        scanf("%d", &requestNb);

        if (requestNb == 1) {
            // Envoi de la requête "1" au serveur
            snprintf(msgBuffer, sizeof(msgBuffer), "1");
            write(toServerTubeFd, msgBuffer, strlen(msgBuffer) + 1);

            // Attente de l'acquittement du serveur
            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromServerTubeFd, msgBuffer, sizeof(msgBuffer) - 1);

            // Envoi de l'index du spectacle
            printf("Enter show index to see remaining places: ");
            fflush(stdout);
            scanf("%d", &showIdx);
            snprintf(msgBuffer, sizeof(msgBuffer), "%d", showIdx);
            write(toServerTubeFd, msgBuffer, strlen(msgBuffer) + 1);

            // Réception et affichage de la réponse
            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromServerTubeFd, msgBuffer, sizeof(msgBuffer) - 1);
            printf("Remaining places: %s\n\n", msgBuffer);

        } else if (requestNb == 2) {
            // Envoi de la requête "2" au serveur
            snprintf(msgBuffer, sizeof(msgBuffer), "2");
            write(toServerTubeFd, msgBuffer, strlen(msgBuffer) + 1);

            // Attente de l'acquittement du serveur
            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromServerTubeFd, msgBuffer, sizeof(msgBuffer) - 1);

            // Envoi de l'index et du nombre de places
            printf("Enter show index and number of places to book (ex: 1 5): ");
            fflush(stdout);
            scanf("%d %d", &showIdx, &nbPlace);
            snprintf(msgBuffer, sizeof(msgBuffer), "%d %d", showIdx, nbPlace);
            write(toServerTubeFd, msgBuffer, strlen(msgBuffer) + 1);

            // Réception et affichage de la réponse
            memset(msgBuffer, 0, sizeof(msgBuffer));
            read(fromServerTubeFd, msgBuffer, sizeof(msgBuffer) - 1);
            printf("Server response: %s\n\n", msgBuffer);

        } else {
            printf("Unknown request: %d\n\n", requestNb);
        }

    }
    close(fromServerTubeFd);
    close(toServerTubeFd);

    return 0;
}

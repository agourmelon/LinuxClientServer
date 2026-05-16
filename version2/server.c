#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
    if (index >= NB_SHOWS) return -1;
    if (nbPlace > (unsigned int)BOOKABLE_SHOWS[index].remainingPlace) return -2;
    BOOKABLE_SHOWS[index].remainingPlace -= nbPlace;
    return nbPlace;
}

int getShowRemainingPlaces(size_t index) {
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
    //Initialisation des variables
    
    //Message de lancement du server.
    //Céation de la file de messages.
    //Boucle infine.
    //Attente d'un messages.
    //Si requete init: envoie message listes spectacles.
    //Si spectacle n'existe pas: envoie erreur spectacle.
    //Si requête de consultation: envoie message place restantes.
    //Si requête de réservation: envoie message confirmation.
    //Destruction de la file de message.

    return 0;
}

#pragma once

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char* title;
    int remainingPlace;
} ShowBookingInfo;

static ShowBookingInfo SHOWS[] = {
    {"Mamma Mia", 100},
    {"The Cid", 100},
    {"West Side Story", 100},
    {"Hamlett", 100}
};

const size_t NB_SHOWS = sizeof(SHOWS) / sizeof(ShowBookingInfo);

// Réserve nbPlace places pour le spectacle à l'indice index. Retourne nbPlace, -1 (indice invalide) ou -2 (places insuffisantes).
static int bookShow(size_t index, unsigned int nbPlace) {
    index-=1; // API 1-indexée → tableau 0-indexé; index=0 déborde à SIZE_MAX ≥ NB_SHOWS → -1
    if (index >= NB_SHOWS) return -1;
    if (nbPlace > (unsigned int)SHOWS[index].remainingPlace) return -2;
    SHOWS[index].remainingPlace -= nbPlace;
    return nbPlace;
}

// Retourne le nombre de places restantes pour le spectacle à l'indice index, ou -1 si l'indice est invalide.
static int checkShow(size_t index) {
    index-=1; // API 1-indexée → tableau 0-indexé; index=0 déborde à SIZE_MAX ≥ NB_SHOWS → -1
    if (index >= NB_SHOWS) return -1;
    return SHOWS[index].remainingPlace;
}

// Remplit buffer avec la liste numérotée des spectacles ("N. titre\n").
// Retourne le nombre de caractères écrits, ou -1 si buffer trop petit.
static int printShowIndexMap(char *buffer, size_t bufferSize) {
    int position = 0;
    for (int i = 0; i < (int)NB_SHOWS; i++) {
        ShowBookingInfo show = SHOWS[i];
        position += snprintf(
            buffer + position,
            bufferSize - position,
            "%d. %-20s\n",
            i + 1, show.title);
        if (position >= (int)bufferSize) return -1;
    }
    return position;
}

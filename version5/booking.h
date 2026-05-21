#pragma once

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char* title;
    int remainingPlace;
} ShowBookingInfo;

static const ShowBookingInfo SHOWS_INIT[] = {
    {"Mamma Mia", 100},
    {"The Cid", 100},
    {"West Side Story", 100},
    {"Hamlett", 100}
};

#define NB_SHOWS (sizeof(SHOWS_INIT) / sizeof(ShowBookingInfo))

static int bookShow(ShowBookingInfo *shows, size_t index, unsigned int nbPlace) {
    index -= 1; // API 1-indexée → tableau 0-indexé; index=0 déborde à SIZE_MAX ≥ NB_SHOWS → -1
    if (index >= NB_SHOWS) return -1;
    if (nbPlace > (unsigned int)shows[index].remainingPlace) return -2;
    shows[index].remainingPlace -= nbPlace;
    return nbPlace;
}

static int checkShow(const ShowBookingInfo *shows, size_t index) {
    index -= 1;
    if (index >= NB_SHOWS) return -1;
    return shows[index].remainingPlace;
}

static int printShowIndexMap(const ShowBookingInfo *shows, char *buffer, size_t bufferSize) {
    int position = 0;
    for (int i = 0; i < (int)NB_SHOWS; i++) {
        position += snprintf(
            buffer + position,
            bufferSize - position,
            "%d. %-20s\n",
            i + 1, shows[i].title);
        if (position >= (int)bufferSize) return -1;
    }
    return position;
}

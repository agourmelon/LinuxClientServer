#pragma once
#include <unistd.h>

enum ResponseType {
    SUCCESS,
    ERROR,
};

typedef struct {
    long mtype;              // type de message MSQ : INIT_MTYPE/CHECK_MTYPE/BOOK_MTYPE pour cibler le bon thread
    unsigned int showIdx;    // indice du spectacle (1-indexé, utilisé pour CHECK et BOOK)
    unsigned int nbPlace;    // nombre de places demandées (utilisé pour BOOK uniquement)
    pid_t clientPid;         // PID du client : le serveur s'en sert pour router la réponse
} Request;


typedef struct {
    long mtype;              // PID du client destinataire, positionné par le serveur
    enum ResponseType rtype; // statut de la réponse (SUCCESS/ERROR)
    char message[1024];      // contenu textuel de la réponse
} Response;


// msgsnd/msgrcv exigent la taille du message sans le champ mtype (premier membre)
static long requestSize = sizeof(Request) - sizeof(long);
static long responseSize = sizeof(Response) - sizeof(long);

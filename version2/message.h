#pragma once
#include <unistd.h>

enum RequestType {
    INIT,
    CHECK,
    BOOK,
};

enum ResponseType {
    SUCCESS,
    ERROR,
};

typedef struct Request {
    long mtype;              // type de message MSQ : SERVER_MTYPE pour cibler le serveur
    enum RequestType rtype;  // type de requête (INIT/CHECK/BOOK)
    unsigned int showIdx;    // indice du spectacle (1-indexé, utilisé pour CHECK et BOOK)
    unsigned int nbPlace;    // nombre de places demandées (utilisé pour BOOK uniquement)
    pid_t clientPid;         // PID du client : le serveur s'en sert pour router la réponse
} Request;


typedef struct Response{
    long mtype;              // PID du client destinataire, positionné par le serveur
    enum ResponseType rtype; // statut de la réponse (SUCCESS/ERROR)
    char message[1024];      // contenu textuel de la réponse
} Response;


// msgsnd/msgrcv exigent la taille du message sans le champ mtype (premier membre)
static long requestSize = sizeof(Request) - sizeof(long);
static long responseSize = sizeof(Response) - sizeof(long);

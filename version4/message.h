#pragma once
#include <unistd.h>

enum ResponseType {
    SUCCESS,
    ERROR,
};

typedef struct {
    long mtype;
    unsigned int showIdx;
    unsigned int nbPlace;
    pid_t clientPid;
} Request;


typedef struct {
    long mtype;
    enum ResponseType rtype;
    char message[1024];
} Response;


// msgsnd/msgrcv exigent la taille du message sans le champ mtype (premier membre)
static long requestSize = sizeof(Request) - sizeof(long);
static long responseSize = sizeof(Response) - sizeof(long);

#pragma once
#include <unistd.h>

enum RequestType {
    INIT,
    CHECK,
    BOOK,
};

typedef struct Request {
    long mtype;
    enum RequestType rtype;
    unsigned int showIdx;
    unsigned int nbPlace;
    pid_t clientPid;
} Request;


typedef struct Response{
    long mtype;
    char message[1024];
} Response;


static long requestSize = sizeof(Request) - sizeof(long);
static long ResponseSize = sizeof(Response) - sizeof(long);

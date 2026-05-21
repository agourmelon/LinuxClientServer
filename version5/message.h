#pragma once
#include <stdio.h>
#include <unistd.h>

enum ResponseType {
    SUCCESS,
    ERROR,
};

enum RequestType {
    INIT,
    CHECK,
    BOOK,
    EXIT,
};

typedef struct {
    enum RequestType rqType;
    int showIdx;
    int nbPlace;
} Request;


typedef struct {
    enum ResponseType rptype;
    char message[1024];
} Response;


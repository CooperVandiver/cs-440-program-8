#ifndef SERVER_H
#define SERVER_H
#include <stdio.h>

enum CONSTANTS {
    PORT = 11044,
    MAX_PENDING = 5,
    PACKET_SIZE = 515,
    PAYLOAD_SIZE = 512
};

void
handleGet(int, char*, int);

void
handlePut(int, char*, int);

void
sigHandler(int);

static volatile sig_atomic_t running;

#endif

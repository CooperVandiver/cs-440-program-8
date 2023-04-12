#ifndef BFTP_H
#define BFTP_H
#include <stddef.h>

enum BFTP_MSG_TYPES {
    BFTP_GET,
    BFTP_PUT,
    BFTP_DAT,
    BFTP_RDY,
    BFTP_ERR
};

size_t
createPacket(char*, size_t, int, char*, size_t);

int
extractType(char*, size_t);

void
extractPayload(char*, size_t, char*, size_t);

#endif

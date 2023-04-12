#include <err.h>
#include <stdio.h>
#include <string.h>

#include "bftp.h"

size_t
createPacket(char *outBuf,
             size_t outBufSize,
             int type,
             char *payload,
             size_t payloadLen
            )
{
    if (outBuf == NULL)
        errx(1, "In \"createPacket,\" \"outBuf\" cannot be null!");
    if (payload == NULL)
        errx(1, "In \"createPacket,\" \"payload\" cannot be null!");

    if (type != BFTP_DAT)
        payloadLen += 1;

    if (outBufSize < payloadLen + 3)
        errx(1, "In \"createPacket,\" \"outBuf\" is not large enougt to hold"
                "the payload!"
            );
    


    memset(outBuf, 0, outBufSize);
    switch (type) {
        case BFTP_GET:
            strcpy(outBuf, "GET");
            break;
        case BFTP_PUT:
            strcpy(outBuf, "PUT");
            break;
        case BFTP_DAT:
            strcpy(outBuf, "DAT");
            break;
        case BFTP_RDY:
            strcpy(outBuf, "RDY");
            break;
        case BFTP_ERR:
            strcpy(outBuf, "ERR");
            break;
        default:
            errx(1, "In \"createPacket,\" invalid BFTP message type given.");
    }

    memmove(outBuf+3, payload, payloadLen);
    return payloadLen + 3;
}

int
extractType(char* buffer, size_t bufferSize)
{
    char typeStr[4];

    if (buffer == NULL)
        errx(1, "In \"extractType,\" \"buffer\" cannot be null!");

    if (bufferSize <= 3) {
        errx(1, "In \"extractType,\" \"buffer\" must be at least 3 bytes "
                "long!"
            );
    }
    typeStr[3] = '\0';
    memcpy(typeStr, buffer, 3);
    if (!strcmp(typeStr, "GET"))
        return BFTP_GET;
    if (!strcmp(typeStr, "PUT"))
        return BFTP_PUT;
    if (!strcmp(typeStr, "DAT"))
        return BFTP_DAT;
    if (!strcmp(typeStr, "RDY"))
        return BFTP_RDY;
    if (!strcmp(typeStr, "ERR"))
        return BFTP_ERR;
    errx(1, "In \"extractType,\" \"buffer\" contains an invalid opcode.");
}

void
extractPayload(char* payload,
               size_t payloadSize,
               char *buffer,
               size_t bufferSize
              )
{
    if (payload == NULL)
        errx(1, "In \"extractPayload,\" \"payload\" cannot be null!");
    if (buffer == NULL)
        errx(1, "In \"extractPayload,\" \"buffer\" cannot be null!");
    if (bufferSize < 3 || payloadSize <= bufferSize - 3)
        errx(1, "In \"extractPayload,\" \"payload\" is not large enough!");
    
    memset(payload, 0, payloadSize);
    memcpy(payload, buffer+3, bufferSize-3); 
}

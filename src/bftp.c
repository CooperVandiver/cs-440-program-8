#include <err.h>
#include <stdio.h>
#include <string.h>

#include "bftp.h"

/*
 * The createPacket function fills the character array "outBuf" with 3 bytes
 * corresponding to message type ("type" parameter corresponds to an enumerated
 * type located in "bftp.h") followed by the bytes from the "payload" buffer.
 * The payload in the created packet is null-terminated if the packet is not of
 * type DAT.
*/
size_t
createPacket(char *outBuf,
             size_t outBufSize,
             int type,
             char *payload,
             size_t payloadLen
            )
{
    /* Verifying that buffers aren't empty. */
    if (outBuf == NULL)
        errx(1, "In \"createPacket,\" \"outBuf\" cannot be null!");
    if (payload == NULL)
        errx(1, "In \"createPacket,\" \"payload\" cannot be null!");

    /* Accounting for null-termination when type is not DAT. */
    if (type != BFTP_DAT)
        payloadLen += 1;

    /* Verifying that outBuf can hold the entire packet. */
    if (outBufSize < payloadLen + 3)
        errx(1, "In \"createPacket,\" \"outBuf\" is not large enough to hold"
                "the payload!"
            );
    
    /* Clearing "outBuf" and adding the first 3 bytes to the buffer. */
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

    /* Moving the contents of "payload" into "outBuf" following its opcode. */
    memmove(outBuf+3, payload, payloadLen);
    return payloadLen + 3;
}

/*
 * The extractType function returns a value corresponding to the enumerated type
 * (found in "bftp.h") that represents one of the valid message types as
 * specified in the BFTP specification. The first 3 bytes of "buffer" represent
 * the packet's message type.
*/
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

    /* 
     * Moving the first three bytes of "buffer" into "typeStr" so that
     * lexicographical comparison can be used to identify the opcode of the
     * packet located in "buffer."
    */
    memset(typeStr, 0, sizeof(typeStr));
    memmove(typeStr, buffer, 3);
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
    return BFTP_INVALID;
}

/*
 * Extracts the payload of the packet located in "buffer" and copies it into
 * the "payload" buffer.
*/
int
extractPayload(char* payload,
               int payloadSize,
               char *buffer,
               int bufferSize
              )
{
    if (payload == NULL)
        errx(1, "In \"extractPayload,\" \"payload\" cannot be null!");
    if (buffer == NULL)
        errx(1, "In \"extractPayload,\" \"buffer\" cannot be null!");
    if (payloadSize < bufferSize - 3) {
		printf("Payload size: %d\nBuffer Size: %d\n", payloadSize, bufferSize);
        errx(1, "In \"extractPayload,\" \"payload\" is not large enough!");
	}
    
    memset(payload, 0, payloadSize);
	if (bufferSize <= 3)
		return 0;
    memcpy(payload, buffer+3, bufferSize-3); 
    return bufferSize-3;
}

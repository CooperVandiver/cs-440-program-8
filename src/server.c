#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bftp.h"
#include "server.h"

int
main(void)
{
    struct sockaddr_in sin;
    socklen_t sin_len;
    struct timeval tv = {0, 250000};
    fd_set rfds;
    size_t len;
    char packet[PACKET_SIZE];
    int s, new_s, msgType;

    signal(SIGTERM, sigHandler);
    signal(SIGINT, sigHandler);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = INADDR_ANY;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        errx(1, "Socket could not be created.");

    if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
        close(s);
        errx(1, "Couldn't bind name to socket.");
    }

    if (listen(s, MAX_PENDING) == -1) {
        close(s);
        errx(1, "Couldn't listen on socket.");
    }
    
    running = 1;
    while (running) {
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        if (select(s+1, &rfds, NULL, NULL, &tv) > 0) {
            sin_len = sizeof(sin);
            if ((new_s = accept(s, (struct sockaddr*)&sin, &sin_len)) == -1) {
                fprintf(stderr, "Couldn't accept client connection.\n");
                goto MAIN_ERR_1;
            }
            while ((len = recv(new_s, packet, sizeof(packet), 0)) > 0) {
                msgType = extractType(packet, len);
                switch (msgType) {
                    case BFTP_GET:
                        handleGet(new_s, packet, len);
                        break;
                    case BFTP_PUT:
                        handlePut(new_s, packet, len);
                        break;
                    default:
                        break;
                }
            }
            close(new_s);
MAIN_ERR_1:
            ;
        }
    }

    return 0;
}

void
handleGet(int s, char *packet, int packetLen) {
    FILE *fp;
    int len;
    char payload[PAYLOAD_SIZE], *errnoMsg;

    len = extractPayload(payload, PAYLOAD_SIZE, packet, packetLen);

    if ((fp = fopen(payload, "r")) == NULL) {
        errnoMsg = strerror(errno);
        len = createPacket(packet,
                           PACKET_SIZE,
                           BFTP_ERR,
                           errnoMsg,
                           strlen(errnoMsg)
                          );
        send(s, packet, len, 0);
        return;
    }

    len = createPacket(packet, PACKET_SIZE, BFTP_RDY, "", 0);
    send(s, packet, len, 0);

    do {
        len = fread(payload, 1, PAYLOAD_SIZE, fp);
        len = createPacket(packet, PACKET_SIZE, BFTP_DAT, payload, len);
        send(s, packet, len, 0);
    } while (len == 515);
    
    fclose(fp);
}

void
handlePut(int s, char *packet, int packetLen) {
    FILE *fp;
    int len;
    char payload[PAYLOAD_SIZE], *errnoMsg;

    len = extractPayload(payload, PAYLOAD_SIZE, packet, packetLen);

    if ((fp = fopen(payload, "w")) == NULL) {
        errnoMsg = strerror(errno);
        len = createPacket(packet,
                           PACKET_SIZE,
                           BFTP_ERR,
                           errnoMsg,
                           strlen(errnoMsg)
                          );
        send(s, packet, len, 0);
        return;
    }

    len = createPacket(packet, PACKET_SIZE, BFTP_RDY, "", 0);
    send(s, packet, len, 0);

    do {
        len = recv(s, packet, PACKET_SIZE, 0);
        len = extractPayload(payload, PAYLOAD_SIZE, packet, len);
        fwrite(payload, 1, len, fp);
    } while (len == 512); 

    fclose(fp);
}

void
sigHandler(int sigraised)
{
    int errnoState = errno;
    switch (sigraised) {
        case SIGTERM:
            running = 0;
            break;
    }
    errno = errnoState;
}

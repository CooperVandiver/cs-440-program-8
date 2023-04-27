/* 
 * Name: Cooper Vandiver
 * Course-Section: CS-440
 * Assignment: Program 8 - Server
 * Date Due: April 14, 2023
 * Collaborators: None
 * Resources: None
 * Description: A server that communicates with a BFTP client. The protocol is
 *              outlined in the CS 440 Program 8 Assignment PDF. This portion of
 *              the program, the server, is responsible for responding to the
 *              client's requests to send and get files via PUT and GET
 *              requests, respectively.
*/
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bftp.h"
#include "server.h"

/*
 * This program is a server that connects to a BFTP client and communicates
 * with said client over TCP using the BFTP protocol. The type of each message
 * sent must be stored within the first 3 bytes of each packet sent (GET, PUT,
 * RDY, ERR, or DAT). Each message also contains a payload that is at most 512
 * bytes long. For DAT messages, every payload must be 512 bytes until the last
 * message is sent; at this point, the rest of the message will be sent in a
 * payload that is smaller than 512 bytes. If there is no content left to be
 * sent, an empty payload will be sent to inform the other party that the
 * sending of data is complete. Communication begins with the client sending a
 * GET or PUT message, along with a file name in the payload. The file will then
 * be opened locally for reading (GET) or writing (PUT). If there is an error in
 * opening the file, no message will be sent and the client will be informed of
 * the error. The server will then respond with a RDY message or an ERR message
 * if it can't open the file on its end. Following this, the party sending the
 * file will begin sending DAT messages until the file is fully sent. After,
 * both the server and client can close their respective files and begin waiting
 * for the next message to be sent.
*/
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

    /* Setting up signal handlers. */
    signal(SIGTERM, sigHandler);
    signal(SIGINT, sigHandler);

    /* Building address data structure. */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Creating socket */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        errx(1, "Socket could not be created.");

    /* Binding name to socket */
    if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
        close(s);
        errx(1, "Couldn't bind name to socket.");
    }

    /* Listening on socket */
    if (listen(s, MAX_PENDING) == -1) {
        close(s);
        errx(1, "Couldn't listen on socket.");
    }
    
    /* Main loop */
    running = 1;
    while (running) {
        /* 
         * The fd_set data structure is mutated by "select." As such, the file
         * descriptors need to be readded to the bit mask after every call.
        */
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);

        /* Checking to see if the file descriptor is ready to be handled. */
        if (select(s+1, &rfds, NULL, NULL, &tv) > 0) {
            /* Accepting client connection when the fd becomes ready. */
            sin_len = sizeof(sin);
            if ((new_s = accept(s, (struct sockaddr*)&sin, &sin_len)) == -1) {
                fprintf(stderr, "Couldn't accept client connection.\n");
                /* 
                 * If the accept fails, we don't want to attempt to receive over
                 * the socket or close it later. Instead, jump to MAIN_ERR_1 to
                 * begin the next iteration of the main loop.
                */
                goto MAIN_ERR_1;
            }
            /*
             * Waiting for initial message from client. If returned value from
             * receive is less than 0, the client has disconnected or some error
             * has been raised. In either case, exit the loop and disconnect.
            */
            while ((len = recv(new_s, packet, sizeof(packet), 0)) > 0) {
                /* 
                 * Checking initial message type. If the type is not GET or PUT
                 * (or if the message is malformed), inform the client of the
                 * error and wait for the next message to be received. If type
                 * is a GET or a PUT, call the correct function.
                */
                msgType = extractType(packet, len);
                switch (msgType) {
                    case BFTP_GET:
                        handleGet(new_s, packet, len);
                        break;
                    case BFTP_PUT:
                        handlePut(new_s, packet, len);
                        break;
                    default:
                        len = createPacket(packet,
                                           sizeof(packet),
                                           BFTP_ERR,
                                           "Invalid message type. Type must be "
                                           "GET or PUT on initial send.",
                                           62
                                    );
                        send(s, packet, len, 0);
                        break;
                }
            }
            /* Closing socket after communication is complete. */
            close(new_s);
MAIN_ERR_1:
            ;
        }
    }

    /* Closing socket used to accept connections before terminating. */
    close(s);

    return 0;
}

/* 
 * The "handleGet" function handles the process of sending a series of DAT
 * packets to the client. Each DAT packet's payload is read from the file, the
 * name of which is contained in the initial payload.
*/
void
handleGet(int s, char *packet, int packetLen) {
    FILE *fp;
    int len;
    char payload[PAYLOAD_SIZE], *errnoMsg;

    /* Extracting filename from the first packet. */
    len = extractPayload(payload, PAYLOAD_SIZE, packet, packetLen);

    /* 
     * Opening file for reading. If unable to do so, use the global "errno"
     * variable to get the cause of the issue with opening the file. The string
     * returned from the "strerror" function will be used to construct the ERR
     * message that will be sent to the client.
    */
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

    /* Sending initial RDY packet to client. */
    len = createPacket(packet, PACKET_SIZE, BFTP_RDY, "", 0);
    send(s, packet, len, 0);

    /* 
     * Reading from file and sending DAT packets to client until a whole packet
     * cannot be constructed. A do-while loop is used to handle the case in
     * which the file is less than 512 bytes long and also to handle the case
     * that the file is some multiple of 512 bytes long, in which case an empty
     * payload will be sent.
    */
    do {
        len = fread(payload, 1, PAYLOAD_SIZE, fp);
        len = createPacket(packet, PACKET_SIZE, BFTP_DAT, payload, len);
        send(s, packet, len, 0);
    } while (len == 515);
    
    /* Closing the file after reading is complete. */
    fclose(fp);
}

/* 
 * The "handlePut" function handles the process of receiving a series of DAT
 * packets from the client. Each DAT packet's payload is written to a file
 * after being received.
*/
void
handlePut(int s, char *packet, int packetLen) {
    FILE *fp;
    int len;
    char payload[PAYLOAD_SIZE], *errnoMsg;

    /* 
     * Getting initial payload from message. This corresponds to the filename of
     * the file that consists of the payloads of the subsequent DAT packets and
     * will be used to name the file being written to on the local machine.
    */
    len = extractPayload(payload, PAYLOAD_SIZE, packet, packetLen);

    /* 
     * Opening file for writing. If it can't be opened, use the global "errno"
     * variable and associated strerror function to correctly inform the client
     * of what caused the error.
    */
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

    /* Creating and sending RDY packet to client. */
    len = createPacket(packet, PACKET_SIZE, BFTP_RDY, "", 0);
    send(s, packet, len, 0);

    /* 
     * Receiving from client until the payload size of the last message received
     * is less than 512. A do-while loop is used to handle the case that the
     * file being received consists of less than 512 bytes.
    */
    do {
        len = recv(s, packet, PACKET_SIZE, 0);
        len = extractPayload(payload, PAYLOAD_SIZE, packet, len);
        fwrite(payload, 1, len, fp);
    } while (len == 512); 

    /* Closing file after use */
    fclose(fp);
}

/* 
 * Handles signals raised by the operating system. If SIGTERM is raised, the
 * global flag "running" is set to false so that the program may terminate
 * gracefully.
*/
void
sigHandler(int sigraised)
{
    int errnoState = errno;

    /* Using a switch statement in case more signals need to be handled. */
    switch (sigraised) {
        case SIGTERM:
            running = 0;
            break;
    }

    errno = errnoState;
}

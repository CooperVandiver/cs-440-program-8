/* 
 * Name: Cooper Vandiver
 * Course-Section: CS-440
 * Assignment: Program 8 - Client 
 * Date Due: April 14, 2023
 * Collaborators: None
 * Resources: None
 * Description: A client that communicates with a BFTP server. The protocol is
 *              is outlined in the CS 440 Program 8 assignment PDF. This
 *              portion of the program, the client, is responsible for reading
 *              user input and sending files to and from the server via GET and
 *              PUT requests.
*/
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bftp.h"
#include "client.h"

/*
 * This program is a client that connects to a BFTP server and communicates
 * with said server over TCP using the BFTP protocol. The type of each message
 * sent must be stored within the first 3 bytes of each packet sent (GET, PUT,
 * RDY, ERR, or DAT). Each message also contains a payload that is at most 512
 * bytes long. For DAT messages, every payload must be 512 bytes until the last
 * message is sent; at this point, the rest of the message will be sent in a
 * payload that is smaller than 512 bytes. If there is no content left to be
 * sent, an empty payload will be sent to inform the other party that the
 * sending of data is complete. Communication begins with the client sending a
 * GET or PUT message, along with a file name in the payload. The file will also
 * be opened locally for reading (PUT) or writing (GET). If there is an error in
 * opening the file, no message will be sent and the user will be informed of
 * the error. The server will then respond with a RDY message or an ERR message
 * if it can't open the file on its end. Following this, the party sending the
 * file will begin sending DAT messages until the file is fully sent. After,
 * both the server and client can close their respective files and begin waiting
 * for the next message to be sent.
*/
int
main(int argc, char **argv)
{
    /* Verifying that one command-line argument was passed. */
    if (argc != 2)
        errx(1, "Exactly one argument (server hostname) must be passed on the "
                "command line!"
            );
    
    const char *hostname = argv[1];
    struct sockaddr_in sin;
    struct hostent *hp;
    size_t msgSize;
    FILE *fp;
    char buf[515], payload[512], cmd[64], *msg;
    int running, s, len, itemsRead, cmdType, msgType;

    /* Allocating memory for use in getline() function. */
    msgSize = 64;
    if ((msg = malloc(sizeof(char) * msgSize)) == NULL)
        errx(1, "Heap allocation failed!");

    /* Getting server information based on hostname. */
    if (!(hp = gethostbyname(hostname))) 
        errx(1, "Failed to lookup host %s", hostname);

    /* Building address data structure. */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* Opening socket. */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        errx(1, "Unable to open socket");

    /* Connecting to server. */
    if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
        close(s);
        errx(1, "Unable to connect to server");
    }

    fp = NULL;
    running = 1;

    /* Main loop of the program. */
    while (running) {
        /* Getting and parsing user input. */
        printf("bftp> ");
        getline(&msg, &msgSize, stdin);
        memset(cmd, 0, sizeof(cmd));
        itemsRead = sscanf(msg, "%63s %511s", cmd, payload);
        cmdType = getCommand(cmd);

        /* User requested to exit. */
        if (cmdType == BYE)
            running = 0;
        
        /* Unrecognized message */
        else if (cmdType == INVALID_MSG) {
            printf("Invalid command entered. Enter \"help\" for options.\n");
        }

        /* User requested help. */
        else if (cmdType == HELP) {
            printf("Basic-ftp commands:\n"
                   "\tGET - download the named file from the basic-ftp server\n"
                   "\tPUT - upload the named file to the basic-ftp server\n"
                   "\tBYE - quit\n"
				  );
        }

        /* User requested to GET or PUT a file. */
        else {

            /* Invalid number of items passed on command line. */
            if (itemsRead < 2) {
                printf("Filename must be provided.\n");
            }

            else {

                /* User requested to PUT a message. */
                if (cmdType == GET) {
                    /* Opening file for writing. */
                    if ((fp = fopen(payload, "w")) == NULL) {
                        printf("File could not be opened:\t%s\n", strerror(errno));
                        goto ERROR;
                    }
                    /* Creating GET packet. */
                    len = createPacket(buf,
                                       sizeof(buf),
                                       BFTP_GET,
                                       payload,
                                       strlen(payload)
                                      );
                    /* Sending request to server. */
                    if (send(s, buf, len, 0) == -1) {
                        printf("Request could not be sent to server.\n");
                        goto ERROR;
                    }
                }

                /* User requested to PUT a file. */
                else {
                    /* Opening file for reading. */
                    if ((fp = fopen(payload, "r")) == NULL) {
                        printf("File could not be opened:\t%s\n", strerror(errno));
                        goto ERROR;
                    }
                    /* Creating PUT packet. */
                    len = createPacket(buf,
                                       sizeof(buf),
                                       BFTP_PUT,
                                       payload,
                                       strlen(payload)
                                      );
                    /* Sending request to server. */
                    if (send(s, buf, len, 0) == -1) {
                        printf("Request could not be sent to server.\n");
                        goto ERROR;
                    }
                }

                /* Receiving RDY or ERR message from server. */
                if ((len = recv(s, buf, sizeof(buf)-1, 0)) == -1) {
                    printf("Could not receive RDY/ERR message from server.\n");
                }

                /* Handling ERR message. */
                if ((msgType = extractType(buf, len)) != BFTP_RDY) {
                    if (msgType == BFTP_ERR) {
                        len = extractPayload(payload,
                                             sizeof(payload),
                                             buf,
                                             len
                                            );
                        /* 
                         * Server should respond with helpful error message in
                         * the payload of its ERR message, so display it to
                         * user.
                        */
                        printf("Server responded with an error: %s", payload);
                        goto ERROR;
                    }

                    /* Edge case where server response message is invalid. */
                    printf("Server's RDY/ERR did not conform to BFTP "
                           "specifications.\n"
                          );
                    len = createPacket(buf,
                                       sizeof(buf),
                                       BFTP_ERR,
                                       "Expected RDY message.",
                                       21
                                      );
                    send(s, buf, len, 0);
                    goto ERROR;
                }

                /* 
                 * Since user requested a file, being receiving data over socket
                 * and writing each payload to the opened file.
                */
                if (cmdType == BFTP_GET) {
                    /* Looping until the received buffer is not full. */
                    do {
                        /* Receiving packet from server. */
                        if ((len = recv(s, buf, sizeof(buf), 0)) == -1) {
                            printf("Could not receive DAT message from "
                                   "server.\n"
                                  );
                            goto ERROR;
                        }
                        /* Validating type of received packet. */
                        if (extractType(buf, len) != BFTP_DAT) {
                            printf("Server's DAT packet did not conform to "
                                   "BFTP specifications.\n"
                                  );
                            len = createPacket(buf,
                                               sizeof(buf),
                                               BFTP_ERR,
                                               "Expected DAT message.",
                                               21
                                              );
                            send(s, buf, len, 0);
                            goto ERROR;
                        }

                        /* Getting payload from packet. */
                        len = extractPayload(payload,
                                             sizeof(payload),
                                             buf,
                                             len
                                            );
                        /* Writing DAT packet's payload to file. */
                        fwrite(payload, sizeof(char), len, fp);
                    } while (len == 512);
                }

                /*
                 * Since user requested to send a file to the server, begin
                 * reading from opened file and sending its contents to the
                 * server in DAT packets.
                */
                else {
                    /* Looping until created packet is less than 515 bytes. */
                    do {
                        /* Reading in from file. */
                        len = fread(payload, sizeof(char), sizeof(payload), fp);

                        /* Building DAT packet. */
                        len = createPacket(buf,
                                           sizeof(buf),
                                           BFTP_DAT,
                                           payload,
                                           len
                                          );
                        
                        /* Sending DAT packet to server. */
                        send(s, buf, len, 0);
                    } while (len == 515);
                }
            }
        }
/* Label to jump to if an error is thrown at any point. */
ERROR:
        /* Closing file pointer if it's not null. */
        if (fp != NULL)
            fclose(fp);
        fp = NULL;
    }
    /* Closing socket and exiting gracefully */
    close(s);
    return 0;
}

/*
 * The getCommand function parses the first word passed to STDIN by the user.
 * An enumerated type is defined in "client.h" to define the possible message
 * types. The libc function strcasecmp is used in lieu of strcmp, as the
 * program 8 assignment overview specifies that case should be ignored.
*/
int
getCommand(char *s)
{
    if (!strcasecmp(s, "get"))
        return GET;
    if (!strcasecmp(s, "put"))
        return PUT;
    if (!strcasecmp(s, "help"))
        return HELP;
    if (!strcasecmp(s, "bye"))
        return BYE;
    return INVALID_MSG;
}

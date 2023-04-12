#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bftp.h"

enum CONSTANTS {
    SERVER_PORT = 11044
};

int
main(void)
{
    const char *hostname = "localhost";
    struct sockaddr_in sin;
    struct hostent *hp;
    int s, len;
    char buf[515], msg[513];

    if (!(hp = gethostbyname(hostname))) 
        errx(1, "Failed to lookup host %s", hostname);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        errx(1, "Unable to open socket");

    if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
        close(s);
        errx(1, "Unable to connect to server");
    }

    memset(buf, 0, sizeof(buf));
    len = createPacket(buf, sizeof(buf), BFTP_GET, "foo.txt", 7);
    if (send(s, buf, len, 0) == -1) {
        close(s);
        errx(1, "Failed to send to server.");
    }
    memset(buf, 0, sizeof(buf));
    if ((len = recv(s, buf, sizeof(buf)-1, 0)) == -1) {
        close(s);
        errx(1, "Failed to receive from server.");
    }
    extractPayload(msg, sizeof(msg)-1, buf, len);

    return 0;
}

#ifndef CLIENT_H
#define CLIENT_H

enum CLIENT_CONSTANTS {
    SERVER_PORT = 11386
};

enum CLIENT_COMMANDS {
    GET,
    PUT,
    HELP,
    BYE,
    INVALID_MSG
};

int
getCommand(char*);

#endif

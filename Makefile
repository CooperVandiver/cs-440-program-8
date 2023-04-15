CC = clang
CFLAGS = -Wall -pedantic -pipe -O2 -g
SRC = src
OBJ = build
INC = include
CLIENTDIR = client
SERVERDIR = server

.PHONY: clean $(CLIENTDIR)/bftp_client

all: $(CLIENTDIR)/bftp_client

$(CLIENTDIR)/bftp_client: $(OBJ)/client.o $(OBJ)/bftp.o
	$(CC) $(CFLAGS) -o $(CLIENTDIR)/bftp_client $(OBJ)/client.o $(OBJ)/bftp.o

$(OBJ)/client.o: $(SRC)/client.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/client.o $(SRC)/client.c -I$(INC)

$(OBJ)/bftp.o: $(SRC)/bftp.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/bftp.o $(SRC)/bftp.c -I$(INC)

clean:
	@rm -f $(OBJ)/* $(SRC)/*.c.swp $(CLIENTDIR)/bftp_client

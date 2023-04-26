CC = clang
CFLAGS = -Wall -pedantic -pipe -O2 -g
SRC = src
OBJ = build
INC = include
CLIENTDIR = client
SERVERDIR = server

.PHONY: clean all

all: $(CLIENTDIR)/bftp_client $(SERVERDIR)/bftp_server

$(SERVERDIR)/bftp_server: $(OBJ)/server.o $(OBJ)/bftp.o
	$(CC) $(CFLAGS) -o $(SERVERDIR)/bftp_server $(OBJ)/server.o $(OBJ)/bftp.o

$(CLIENTDIR)/bftp_client: $(OBJ)/client.o $(OBJ)/bftp.o
	$(CC) $(CFLAGS) -o $(CLIENTDIR)/bftp_client $(OBJ)/client.o $(OBJ)/bftp.o

$(OBJ)/server.o: $(SRC)/server.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/server.o $(SRC)/server.c -I$(INC)

$(OBJ)/client.o: $(SRC)/client.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/client.o $(SRC)/client.c -I$(INC)

$(OBJ)/bftp.o: $(SRC)/bftp.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/bftp.o $(SRC)/bftp.c -I$(INC)

clean:
	@rm -f $(OBJ)/* $(SRC)/*.c.swp $(CLIENTDIR)/bftp_client

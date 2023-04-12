CC = clang
CFLAGS = -Wall -pedantic -pipe -O2 -g
SRC = src
OBJ = build
INC = include

.PHONY: clean

client: $(OBJ)/client.o $(OBJ)/bftp.o
	$(CC) $(CFLAGS) -o client $(OBJ)/client.o $(OBJ)/bftp.o

$(OBJ)/client.o: $(SRC)/client.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/client.o $(SRC)/client.c -I$(INC)

$(OBJ)/bftp.o: $(SRC)/bftp.c
	$(CC) $(CFLAGS) -c -o $(OBJ)/bftp.o $(SRC)/bftp.c -I$(INC)

clean:
	@rm -f $(OBJ)/* $(SRC)/*.c.swp client

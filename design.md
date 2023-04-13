# Design Document - Basic FTP Server and Client

Cooper Vandiver\
CS 440\
April 21, 2023

Within is described the design specifications for a server and client
implementing the Basic File Transfer Protocol as outlined in the program
8 instruction page. The client and server communicate using TCP, and each
message is given a 3-byte opcode to specify the purpose of said message. The message types are as follows:
* GET - The client is requesting a file from the server. The filename is
stored within the payload.
* PUT - The client is requesting to send a file to the server. The
filename is stored within the payload.
* DAT - Some (or all, given a sufficiently small file) is stored within
the payload of the message.
* RDY - The server signals that it is ready for the communication of file
data to commence. Payload can be empty or arbitrary.
* ERR - An error has occurred somewhere.

# Program Requirements
The program implements a drastically simplified FTP protocol. Files must
be able to be sent back and forth between the client and server's current
directories. Because files are being sent back and forth, the server and
client (along with their associated files) must be in separate
directories.

Functions and constants relevant to both the server and client must be
written independently. The headers/constants and implementation will be
stored in bftp.h and bftp.c respectively. The server and client will
both include the bftp header and link to the compiled bftp.c (bftp.o).

## Client
The client will work based on user input. Four commands must be able to be
parsed from command-line input (GET, PUT, HELP, and BYE). If the user
enters a GET command, the client will request the subsequent argument
(taken to be a filename) from the server and save it to the current 
directory. If the user enters a PUT command, the client will attempt to
send a file of the argument's name to the server from the current
directory. The HELP command will provide the user with a list of options,
and the BYE command will terminate the program gracefully. The client
must have the ability to send multiple requests (via GET or PUT) per
session.

## Server
The server will work based on requests received from the client. The
server will respond to GET requests by sending a file from the current
directory, and it will respond to PUT requests by saving the file sent in
messages of type DAT to the current directory using the specified file
name. If file access fails for any reason (i.e., file not found), the
server must respond with an informative error message.

# Program Inputs

## Client
The client will receive one argument on the command line. This argument is
the hostname of the basic FTP server.

The client will also receive user input. As outlined above, there are four
user commands read from STDIN that the client must respond to and use:
* GET [filename] - requests a file named \<filename\> from the server
* PUT [filename] - attempts to send a file from the current directory
named \<filename\> to the server
* HELP - displays an informative message to the user with instructions on
how to use the client service
* BYE - gracefully terminates the program

The client will also receive some inputs from the server. Following some
request being sent (GET or PUT) the server will respond with a RDY
message before any data can be sent or received. In the event of a PUT
request being sent, the client process will need to receive the file
contents from the DAT message(s) that are sent from the client. If the
server responds with an ERR message, the client will need to display some
informative message to the user.

## Server
The server will listen to requests from the user. GET and PUT messages
will be used to determine whether files will be sent or received, and DAT
messages will be used to save files to the current directory in the event
of a PUT request.

The server does not accept any command-line or console input.

# Program Outputs
Both the server and client will share output in the form of sending
outgoing messages over TCP along with writing files to the disk.

## Client
The client will provide informative feedback to the user based on any
error messages that arise (either locally or from the server) along with
any issues that come with ill-formed command-line commands.

The client will send requests (GET or PUT), data (DAT), and error messages
(ERR) to the server.

The client will write files received from the server following a GET
request to the disk.

## Server

# Test Plan
Because the client will be completed before the server, it will be tested
using the netcat program.

## Client
As the client functions based off user input from STDIN, test cases will
be presented based on user input, generated request (if applicable), and
finally the output of the command (what is displayed to the user and/or
the file system action performed by the process).

| Input | Request | Output |
| ----- | ------- | ------ |
| GET foo.txt | GETfoo.txt | "foo.txt" is either received from the server, or the appropriate error is displayed to the user. |
| PUT business.pdf | PUTbusiness.pdf | "business.pdf" is sent to the server, or the user is informed of why this action failed. |
| GET cat.png | GETcat.png | In this example, "cat.png" does not exist on the server. As such, an ERR message is sent back, and the user is informed of this.|
| PUT data.csv | | In this example, "data.csv" does not exist on the client's machine. As such, no request is sent. |
| HELP | | A help message is displayed to the user, giving an overview of the program and possible commands. |

## Server
The server functions solely off of requests sent from the client. The test
cases will be built based off request sent by the client and the action
that should happen following this request.
| Request | Response Action |
| ------- | --------------- |
| GETfoo.txt | Send a RDY message, then begin sending the contents of "foo.txt" in DAT messages with 512 byte payloads until the end of the file is reached. |
| PUTbusiness.pdf | Send a RDY message, the begin writing the payloads of the incoming DAT messages to "business.pdf." When a message is received that is smaller than 512 bytes, stop receiving and close the file. |
| GETcat.png | In this example, "cat.png" does not exist. As such, respond to the client with an ERR message and cease communication. |

# Solution Plan

## Client
1. Validate that one command-line argument was passed to the process.
2. Generate a socket based on hostname (command-line argument) and connect to the server.
3. Prompt the user for input.
4. Begin receiving user input, parsing the valid command (GET, PUT, HELP, BYE) from each line of input.

### GET
5. Validate that the user also passed a second argument to this command that will correspond to a filename on the server.
6. Open the file passed as an argument for writing. If unable to do so, inform the user and return to step 4.
7. Send request to the server, with the first 3 bytes of the message being "GET," and the filename as the payload.
8. Receive the next message from the server. If the message is of type RDY, proceed. If the message is of type ERR, inform the user and return to step 4.
9. Begin writing the payload of each DAT packet's message to the file specified in the initial GET command.
10. When a message smaller than 515 bytes is received, write the final message to the file (if present), and close the file.
11. Close the file and return to step 4.

### PUT
5. Validate that the user also passed a second argument to this command that will correspond to a local filename.
6. Open the file passed on the command-line. If unable to do so, inform the user and return to step 4.
7. Send request to the server, with the first 3 bytes of the message being "PUT," and the filename as the payload.
8. Receive the next message from the server. If the message is not of type RDY, inform the user of the error and return to step 4. Else, proceed.
9. Begin buffering the input from the opened file, and sending the contents in chunks that are at most 512 bytes.
10. When less than 512 bytes are read from the file, send the last message and cease communication. If the file is some multiple of 512 byte long (i.e. 512, 1024, 2048, etc.), send a DAT message with an empty payload to inform the server that the file has been fully sent.
11. Close the file and return to step 4.

### HELP
5. Write some informative help message to the user.
6. Return to step 4.

### BYE
5. Tear down the connection to the server.
6. Exit gracefully.

## Server
1. Create, bind, and listen over socket.
2. Use select to poll the socket, waiting for it to become ready. If SIGTERM is raised, instead close the main socket and exit gracefully.
3. Accept connection over socket
4. Receive initial request from client and use the first 3 bytes to determine the type of the request.

### GET
5. Open the file using the payload of the message. If unable to do so, send an appropriate ERR message and return to step 4.
6. Send a RDY message to the client.
7. Begin buffering the input from the opened file, and sending the contents in chunks that are at most 512 bytes.
8. When less than 512 bytes are read from the file, send the last message and cease communication. If the file is some multiple of 512 byte long (i.e. 512, 1024, 2048, etc.), send a DAT message with an empty payload to inform the client that the file has been fully sent.
9. Close the file and return to step 4.

### PUT
5. Open the file using the payload of the message for writing. If unable to do so, send an appropriate ERR message and return to step 4.
6. Send a RDY message to the client.
7. Begin receiving DAT messages from the client, writing each payload into the opened file.
8. When the last DAT message received is less than 515 bytes long, stop receiving.
9. Close the file and return to step 4.


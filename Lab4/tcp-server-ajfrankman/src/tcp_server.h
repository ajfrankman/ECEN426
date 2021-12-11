#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define TCP_SERVER_DEFAULT_PORT "8083"
#define TCP_SERVER_BAD_SOCKET -1
#define TCP_SERVER_BACKLOG 10
#define TCP_SERVER_ERROR_MESSAGE "error"

// Contains all of the information needed to create to connect to the server and
// send it a message.
typedef struct Config {
    char *port;
} Config;

typedef struct Request {
    char *action;
    char *message;
} Request;

typedef struct Response {
    char *message;
} Response;

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_parse_arguments(int argc, char *argv[], Config *config);

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Create and bind to a server socket using the provided configuration.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_server_create(Config config);

/*
Description:
    Listen on the provided server socket for incoming clients. When a client connects, return the
    client socket file descriptor. *This is a blocking call.*
Arguments:
    int socket: The server socket to accept on.
Return value:
    Returns the client socket file descriptor or -1 if an error occurs.
*/
int tcp_server_accept(int socket);

/*
Description:
    Read data from the provided client socket, parse the data, and fill in the Request struct.
    The buffers contained in the Request struct must be freed using tcp_server_client_cleanup.
Arguments:
    int socket: The client socket to read from.
    Request* request: The request struct that will be filled in.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_receive_request(int socket, Request *request);

/*
Description:
    Sends the provided Response struct on the provided client socket.
Arguments:
    int socket: The client socket to send the data with.
    Response response: The struct containing the response data.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_send_response(int socket, Response response);

/*
Description:
    Cleans up allocated resources and sockets.
Arguments:
    int socket: The client socket to close.
    Request request: The strcut to clean up.
    Response response: The struct to clean up.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_client_cleanup(int socket, Request request, Response response);

/*
Description:
    Cleans up allocated resources and sockets.
Arguments:
    int socket: The server socket to close.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_cleanup(int socket);

///////////////////////////////////////////////////////////////////////
////////////////////// PROTOCOL RELATED FUNCTIONS /////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Convert a Request struct into a Response struct. This function will allocate the necessary
    buffers to fill in the Response struct. The buffers contained in the Response struct must be
    freeded using tcp_server_client_cleanup.
Arguments:
    Request request: The request struct that will be processed.
    Response *response: The response struct that will be filled in.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_process_request(Request request, Response *response);

#endif
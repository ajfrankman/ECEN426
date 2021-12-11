#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HTTP_SERVER_DEFAULT_PORT "8085"
#define HTTP_SERVER_DEFAULT_RELATIVE_PATH "."
#define HTTP_SERVER_BAD_SOCKET -1
#define HTTP_SERVER_BACKLOG 10
#define HTTP_SERVER_HTTP_VERSION "HTTP/1.1"
#define HTTP_SERVER_MAX_HEADER_SIZE 512
#define HTTP_SERVER_FILE_CHUNK 1024

// Contains all of the information needed to create to connect to the server and
// send it a message.
typedef struct Config {
    char *port;
    char *relative_path;
    bool delay;
} Config;

typedef struct Header {
    char *name;
    char *value;
} Header;

typedef struct Request {
    char *method;
    char *path;
    int num_headers;
    Header **headers;
} Request;

typedef struct Response {
    char *status;
    FILE *file;
    int num_headers;
    Header **headers;
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
int http_server_parse_arguments(int argc, char *argv[], Config *config);

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
int http_server_create(Config config);

/*
Description:
    Listen on the provided server socket for incoming clients. When a client connects, return the
    client socket file descriptor. *This is a blocking call.*
Arguments:
    int socket: The server socket to accept on.
Return value:
    Returns the client socket file descriptor or -1 if an error occurs.
*/
int http_server_accept(int socket);

/*
Description:
    Read data from the provided client socket, parse the data, and fill in the Request struct.
    The buffers contained in the Request struct must be freed using http_server_client_cleanup.
Arguments:
    int socket: The client socket to read from.
    Request* request: The request struct that will be filled in.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_receive_request(int socket, Request *request);

/*
Description:
    Sends the provided Response struct on the provided client socket.
Arguments:
    int socket: The client socket to send the data with.
    Response response: The struct containing the response data.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_send_response(int socket, Response response);

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
int http_server_client_cleanup(int socket, Request request, Response response);

/*
Description:
    Cleans up allocated resources and sockets.
Arguments:
    int socket: The server socket to close.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_cleanup(int socket);

///////////////////////////////////////////////////////////////////////
////////////////////// PROTOCOL RELATED FUNCTIONS /////////////////////
///////////////////////////////////////////////////////////////////////

// A helper function to be used inside of http_server_receive_request. This should not be used
// directly in main.c.
/*
Description:
    Converts a string into a request struct. A helper function to be used
    inside of http_server_receive_request. This should not be used directly
    in main.c.
Arguments:
    char *buf: The string containing the request.
    Request *request: The request struct that will be filled in by buf.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_parse_request(char *buf, Request *request);

/*
Description:
    Convert a Request struct into a Response struct. This function will allocate the necessary
    buffers to fill in the Response struct. The buffers contained in the Response struct must be
    freeded using http_server_client_cleanup.
Arguments:
    Request request: The request struct that will be processed.
    char *relative_path: The path to server the files from.
    Response *response: The response struct that will be filled in.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_process_request(Request request, char *relative_path, Response *response);

#endif

#include "http_server.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#define ARG_NUM 0
#define DEFAULT_PORT "8084"
#define MAX_PATH_LENGTH 256
#define TO_MANY_HEADERS 10000000

char helpMessage[150] = "\n\nUsage: tcp_server [--help] [-v] [-p PORT]\n\n"

                        "Options:"
                        "  --help\n"
                        "  -v, --verbose\n"
                        "  --port PORT, -p PORT\n"
                        "  --folder FOLDER, -f FOLDER\n\n";

struct addrinfo hints, *servinfo, *p;

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/*
Description:
    Make sure the message length valid.
Arguments:
    char *messageLength: the string represenation of the length of the message.
Return value:
    Returns true on success, and false if that space doesn't exist
exist.
*/
bool checkStringIsNum(char *messageLength) {
    for (int i = 0; i < (int)strlen(messageLength); i++) {
        if (!isdigit(messageLength[i])) {
            return false;
        }
    }
    return true;
}

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
int http_server_parse_arguments(int argc, char *argv[], Config *config) {
    int option;
    bool portSet = 0;
    bool folderSet = 0;
    log_set_quiet(true);

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                               {"port", required_argument, 0, 'p'},
                                               {"verbose", no_argument, 0, 'v'},
                                               {"folder", required_argument, 0, 'f'},
                                               {0, 0, 0, 0}};

        option = getopt_long(argc, argv, ":vp:f:h", long_options, &option_index);
        if (option == -1)
            break;

        switch (option) {
        case 'h':
            printf("%s", helpMessage);
            break;
        case 'p':
            if (checkStringIsNum(optarg) == false) {
                printf("%s", helpMessage);
                return 1;
            }
            config->port = malloc(strlen(optarg) + 1);
            sprintf(config->port, "%s", optarg);
            portSet = 1;
            break;
        case 'v':
            log_set_quiet(false);
            break;
        case 'f':
            folderSet = 1;
            config->relative_path = malloc(strlen(optarg) + 1);
            sprintf(config->relative_path, "%s", optarg);
            break;
        case ':': // Missing option argument
            log_error("Missing option argument\n\n");
            printf("%s", helpMessage);
            return 1;
            break;
        case '?': // Invalid option
            log_error("Invalid option\n\n");
            printf("%s", helpMessage);
            return 1;
            break;
        default:
            break;
        }
    }
    log_info("Parsing arguments.");
    if ((argc - optind) != ARG_NUM) {
        log_error("Invalid argument ammount\n\n");
        printf("%s\n\n", helpMessage);
        return 1;
    }

    // If port and host were not specified, set to default
    if (portSet == 0) {
        log_info("Setting default port: %s.", DEFAULT_PORT);
        config->port = DEFAULT_PORT;
    }

    if (folderSet == 0) {
        config->relative_path = ".";
    }

    return 0;
}

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
int http_server_create(Config config) {

    int sockfd; // listen on sock_fd.
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, config.port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_error("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            log_error("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            log_error("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        log_error("server: failed to bind\n");
        exit(1);
    }

    return sockfd;
}

/*
Description:
    Listen on the provided server socket for incoming clients. When a client connects, return the
    client socket file descriptor. *This is a blocking call.*
Arguments:
    int socket: The server socket to accept on.
Return value:
    Returns the client socket file descriptor or -1 if an error occurs.
*/
int http_server_accept(int socket) {
    int new_fd;

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    printf("server: waiting for connections...\n");

    if (listen(socket, HTTP_SERVER_BACKLOG) == -1) {
        log_error("listen");
        return -1;
    }

    sin_size = sizeof their_addr;
    new_fd = accept(socket, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        return new_fd;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);

    return new_fd;
}

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
int http_server_receive_request(int socket, Request *request) {

    request->num_headers = -1; // First part is a method not a header
    int bytesReceived;
    int totalBytesRecevied = 0;
    int request_buf_size = 500;
    char *requestBuf;
    requestBuf = (char *)malloc(request_buf_size * sizeof(char));
    int temp_buf_size = 500;
    char *tempBuf;
    tempBuf = (char *)malloc(temp_buf_size * sizeof(char));

    while (true) {
        if ((bytesReceived = recv(socket, tempBuf, 1, 0)) == -1) {
            log_error("Did not receive all data.\n\n");
            return 1;
        }
        memcpy(requestBuf + totalBytesRecevied, tempBuf, bytesReceived);
        totalBytesRecevied += bytesReceived;
        requestBuf[totalBytesRecevied] = '\0';

        // Reallocate buffer if it is half full
        if (strlen(requestBuf) > (size_t)(request_buf_size / 2)) {
            request_buf_size = request_buf_size * 5;
            requestBuf = (char *)realloc(requestBuf, request_buf_size * sizeof(char));
        }

        if (requestBuf[totalBytesRecevied - 1] == '\n') {
            request->num_headers += 1;
        }
        if (requestBuf[totalBytesRecevied - 1] == '\n' &&
            requestBuf[totalBytesRecevied - 2] == '\r' &&
            requestBuf[totalBytesRecevied - 3] == '\n' &&
            requestBuf[totalBytesRecevied - 4] == '\r') {

            log_info("Found the end of the request. Parsing...");

            break;
        }
    }

    return (http_server_parse_request(requestBuf, request));
}

/*
Description:
    Sends the provided Response struct on the provided client socket.
Arguments:
    int socket: The client socket to send the data with.
    Response response: The struct containing the response data.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_send_response(int socket, Response response) {
    printf("server: sending beginning\n");

    char *statusLine;
    char *header;
    unsigned long totalSentBytes = 0;
    int chunkSize = 1024;

    // Send Status Line
    statusLine = malloc(12 + strlen(response.status));
    sprintf(statusLine, "HTTP/1.1 %s\r\n", response.status);
    if (send(socket, statusLine, strlen(statusLine), 0) < (ssize_t)strlen(statusLine)) {
        log_error("Could not send statusLine");
        return 1;
    }

    header = malloc(strlen(response.headers[0]->name) + strlen(response.headers[0]->value) + 8);
    sprintf(header, "%s: %s\r\n\r\n", response.headers[0]->name, response.headers[0]->value);
    if (send(socket, header, strlen(header), 0) < (ssize_t)strlen(header)) {
        log_error("Could not send header");
        return 1;
    }

    // Get size of file
    fseek(response.file, 0, SEEK_END);
    unsigned long file_length = (unsigned long)ftell(response.file);
    // Go back to the beginning
    fseek(response.file, 0, SEEK_SET);

    // int outerLoop = 0;
    // int innerLoop = 0;
    while (true) {
        char tempBuf[chunkSize];
        int sentAmount = 0;
        int readAmount = 0;
        int sendValue = 0;
        readAmount = fread(tempBuf, sizeof(char), chunkSize, response.file);

        // printf("Send ammount outer: %d\n", outerLoop);
        // outerLoop++;
        while (sentAmount < readAmount) {
            // printf("Send ammount inner: %d\n", innerLoop);
            // innerLoop++;
            sendValue = send(socket, tempBuf + sentAmount, readAmount - sentAmount, 0);

            if (sendValue != -1) {
                sentAmount += sendValue;
            } else if (sendValue == -1) {
                printf("NEGATIVE 1\n");
                return 0;
            }
        }
        totalSentBytes += readAmount;

        log_info("file_length: %d  totalSentBytes: %d\n", file_length, totalSentBytes);

        if (totalSentBytes == file_length) {
            printf("OOPS\n");
            return 0;
        }
    }

    return 1;
}

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
int http_server_client_cleanup(int socket, Request request, Response response) {
    log_info("Cleaning Client");
    if (socket != -1) {
        close(socket);
    }

    log_info("request.num_headers = %d", request.num_headers);

    for (int i = 0; i < request.num_headers - 2; i++) {
        if (request.headers[i]->name != NULL)
            free(request.headers[i]->name);
        if (request.headers[i]->value != NULL)
            free(request.headers[i]->value);
        if (request.headers[i] != NULL)
            free(request.headers[i]);
    }
    if (request.headers != NULL)
        free(request.headers);

    if (request.path != NULL)
        free(request.path);
    if (request.method != NULL)
        free(request.method);

    for (int i = 0; i < response.num_headers; i++) {
        if (response.headers[i]->name != NULL)
            free(response.headers[i]->name);
        if (response.headers[i]->value != NULL)
            free(response.headers[i]->value);
        if (response.headers[i] != NULL)
            free(response.headers[i]);
    }

    log_info("Done freeing");

    if (response.file != NULL) {
        fclose(response.file);
    }
    response.file = NULL;

    if (response.status != NULL)
        free(response.status);

    return 0;
}

/*
Description:
    Cleans up allocated resources and sockets.
Arguments:
    int socket: The server socket to close.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int http_server_cleanup(int socket) { return (close(socket)); }

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
int http_server_parse_request(char *requestBuf, Request *request) {

    char *beginLine = NULL;
    char *value = NULL;
    char *endLine = NULL;

    // Set Method
    beginLine = requestBuf;
    endLine = strchr(beginLine, ' ');
    request->method = malloc(endLine - beginLine + 1);
    memcpy(request->method, beginLine, endLine - beginLine);
    request->method[endLine - beginLine] = '\0';

    // Set Path
    beginLine = endLine + 1;
    endLine = strchr(beginLine, ' ');
    request->path = malloc(endLine - beginLine + 1);
    memcpy(request->path, beginLine, endLine - beginLine);
    request->path[endLine - beginLine] = '\0';

    endLine = strchr(beginLine, '\n');

    request->headers = malloc(sizeof(Header *) * request->num_headers);

    int nameLength;
    int valueLength;
    for (int i = 0; i < TO_MANY_HEADERS; i++) {
        beginLine = endLine + 1;
        value = strchr(beginLine, ':') + 2;
        endLine = strchr(beginLine, '\n');

        log_info("beginLine to value:%.*s", value - beginLine, beginLine);
        log_info("value to endLine:%.*s", endLine - value, value);

        request->headers[i] = malloc(sizeof(Header));

        nameLength = value - beginLine - 2;
        request->headers[i]->name = malloc(nameLength + 1);
        memcpy(request->headers[i]->name, beginLine, nameLength);
        request->headers[i]->name[value - 2 - beginLine] = '\0';

        valueLength = endLine - value;
        request->headers[i]->value = malloc(valueLength + 1);
        memcpy(request->headers[i]->value, value, valueLength);
        request->headers[i]->value[endLine - value] = '\0';

        log_info("This is request->headers[%d]->name: %s", i, request->headers[i]->name);
        log_info("This is request->headers[%d]->value: %s", i, request->headers[i]->value);

        // Alternatively I could add a counter that checks if we have done the correct number of
        // headers. That may be a cleaner option.

        if (beginLine[endLine - beginLine - 1] == '\r' && endLine[0] == '\n' &&
            endLine[1] == '\r' && endLine[2] == '\n') {
            return 0;
        }
    }
    return 1;
}

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
int http_server_process_request(Request request, char *relative_path, Response *response) {

    char status[10];
    FILE *myFile;
    char fileLengthString[100];

    int fullPathLength = strlen(relative_path) + strlen(request.path);
    char fullPath[fullPathLength];
    sprintf(fullPath, "%s%s", relative_path, request.path);
    printf("fullPath: %s\n", fullPath);

    if (strcmp(request.method, "GET") != 0) {
        if ((myFile = fopen("www/405.html", "r")) == NULL)
            return 1;
        log_error("Method Not Allowed");
        sprintf(status, "%d", 404);
        response->status = calloc(1, 10);
        memcpy(response->status, status, strlen(status));
    } else if ((myFile = fopen(fullPath, "r")) != NULL) {
        sprintf(status, "%d", 200);
        response->status = calloc(1, 10);
        memcpy(response->status, status, strlen(status));
    } else {
        if ((myFile = fopen("www/404.html", "r")) == NULL)
            return 1;
        log_error("Could not open file.");
        sprintf(status, "%d", 404);
        response->status = calloc(1, 10);
        memcpy(response->status, status, strlen(status));
    }
    response->file = malloc(sizeof(FILE *));
    response->file = myFile;

    // Get size of file
    fseek(response->file, 0, SEEK_END);
    unsigned long file_length = (unsigned long)ftell(response->file);
    // Go back to the beginning
    fseek(response->file, 0, SEEK_SET);

    // Set header name and value
    response->headers = malloc(sizeof(Header *));
    response->headers[0] = malloc(sizeof(Header));
    response->headers[0]->name = calloc(1, 15);
    memcpy(response->headers[0]->name, "Content-Length", 14);
    sprintf(fileLengthString, "%lu", file_length);
    response->headers[0]->value = malloc(strlen(fileLengthString) + 1);
    memcpy(response->headers[0]->value, fileLengthString, strlen(fileLengthString));
    response->headers[0]->value[strlen(fileLengthString)] = '\0';

    // For now I will only deal with one header.
    response->num_headers = 1;
    return 0;
}
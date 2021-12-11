#include "tcp_server.h"
#include "log.h"
#include "stringManipulation.h"

char helpMessage[150] = "\n\nUsage: tcp_server [--help] [-v] [-p PORT]\n\n"

                        "Options:"
                        "  --help\n"
                        "  -v, --verbose\n"
                        "  --port PORT, -p PORT\n\n";

#define ARG_NUM 0
#define DEFAULT_PORT "8083"
#define SA struct sockaddr
#define ACTION_LENGTH_BYTES 4
#define MAX_ACTION_LENGTH 11

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
int tcp_server_parse_arguments(int argc, char *argv[], Config *config) {
    int option;
    bool portSet = 0;
    log_set_quiet(true);

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                               {"port", required_argument, 0, 'p'},
                                               {"verbose", no_argument, 0, 'v'},
                                               {0, 0, 0, 0}};

        option = getopt_long(argc, argv, ":vp:h", long_options, &option_index);
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
            sprintf(config->port, "%s", optarg);
            portSet = 1;
            break;
        case 'v':
            log_set_quiet(false);
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
        log_info("Setting default port: \"8083\".");
        config->port = DEFAULT_PORT;
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
int tcp_server_create(Config config) {

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
int tcp_server_accept(int socket) {

    int new_fd;

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    printf("server: waiting for connections...\n");

    if (listen(socket, TCP_SERVER_BACKLOG) == -1) {
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
    Given a string, check if it is a in fact an action.
Arguments:
    char *action: string that may or not be an action.
Return value:
    Returns true on success, and 0 if action is invalid.
exist.
*/
bool checkValidAction(char *action) {
    if (strcmp(action, "lowercase") == 0)
        return true;
    else if (strcmp(action, "title-case") == 0)
        return true;
    else if (strcmp(action, "reverse") == 0)
        return true;
    else if (strcmp(action, "uppercase") == 0)
        return true;
    else if (strcmp(action, "shuffle") == 0)
        return true;
    else
        return false;
}

/*
Description:
    Find the nth space character in a string.
Arguments:
    char *message: The current input from the server.
    int totalBytesReceived: total byts in message.
    int nthSpace: What number space do you want.
    char* spacePointer: This is a Null pointer that can be set if the correct space exists.
Return value:
    Returns 1 on success, and 0 if that space doesn't exist
exist.
*/
int findSpace(char *message, uint32_t totalBytesReceived, int nthSpace, char **spacePointer) {
    int spaceCounter = 0;

    for (int i = 0; i < (int)totalBytesReceived; i++) {
        if (message[i] == ' ') {
            spaceCounter++;
            if (spaceCounter == nthSpace) {
                *spacePointer = &message[i];
                return 1;
            }
        }
    }
    return 0;
}

/*
Description:
    Returns length of full message including header number if it exists in buffer.
Arguments:
    char *message: The current input from the server.
    int totalBytes: total byts in message.
Return value:
    Returns a message length including header length on success, 0 if a full message doesn't
exist, and -1 if an error is detected.
*/
int fillRequest(char *message, uint32_t totalBytesReceived, Request *request) {

    char *spaceOne = NULL;
    char *spaceTwo = NULL;
    char *userMessage = NULL;

    if ((findSpace(message, totalBytesReceived, 1, &spaceOne) == 1) &&
        (findSpace(message, totalBytesReceived, 2, &spaceTwo) == 1)) {
        userMessage = spaceTwo + 1;
        log_info("Message header exists. Checking for rest of message.");
        char *action;
        // actionLength should be the number of bytes from the beginning of message to the first
        // space.
        int actionLength = spaceOne - message;
        action = malloc(actionLength + 1);
        memcpy(action, message, (spaceOne - message));
        action[actionLength] = '\0';

        char lengthString[10];
        int messageLength = atoi(spaceOne);
        if (messageLength == 0) {
            log_error("messageLength not a number or was zero.");
            printf("Message length must be a number greater than 0.");
            printf("%s", helpMessage);
            free(action);
            return -1;
        }
        sprintf(lengthString, "%d", messageLength);

        /* The string from the client will be in the format below.
                ACTION MESSAGELENGTH MESSAGE
               |   1 ||      3     ||   4  |
            Action followed by a single whitespace, followed by the messagelength followed by
           another whitespace and then the message.

           When totalBytesReceived is >= all the characters (or bytes) in the message, then you have
           a message. Take that message and parse it.
        */
        int requestLength = actionLength + 2 + strlen(lengthString) + messageLength;
        if ((int)totalBytesReceived >= (int)requestLength) {
            if (checkValidAction(action) == false) {
                log_error("\"%s\" is not a valid action.", action);
                printf("\nINVALID ACTION: \"%s\"", action);
                printf("%s", helpMessage);
                free(action);
                return -1;
            }
            if (checkStringIsNum(&lengthString[0]) == false) {
                printf("\nINVALID MESSAGE LENGTH: \"%s\"", lengthString);
                printf("%s", helpMessage);
                free(action);
                return -1;
            }
            char *shouldBeSpace = (spaceOne + strlen(lengthString) + 1);
            if (shouldBeSpace[0] != ' ') {
                log_error("Found character in messagelength section.");
                printf("\nMESSAGE LENGTH SHOULD BE SEPERATED FROM MESSAGE BY A SPACE\n");
                printf("%s", helpMessage);
                free(action);
                return -1;
            }

            log_info("Setting request action.");
            request->action = malloc(strlen(action) + 1);
            memcpy(request->action, action, strlen(action));
            request->action[strlen(action)] = '\0';

            log_info("Setting request message");
            request->message = malloc(messageLength + 1);
            memcpy(request->message, userMessage, messageLength);
            request->message[messageLength] = '\0';

            log_info("fillRequest/Request.action: %s\n", request->action);
            log_info("fillRequest/Request.message: %s\n", request->message);

            free(action);
            return requestLength;
        }
        log_info("Need to receive more bytes.");
    } else {
        log_info("Not enough whitespaces.");
        log_error("Invalid message length.");
        printf("%s", helpMessage);
        return -1;
    }
    return 0;
}

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
int tcp_server_receive_request(int socket, Request *request) {
    int bytesReceived;
    int totalBytesRecevied = 0;
    int main_buf_size = 500;
    char *mainBuf;
    mainBuf = (char *)malloc(main_buf_size * sizeof(char));
    int temp_buf_size = 500;
    char *tempBuf;
    tempBuf = (char *)malloc(temp_buf_size * sizeof(char));
    int fillRequestResult;

    while (true) {
        if ((bytesReceived = recv(socket, tempBuf, main_buf_size - totalBytesRecevied, 0)) == -1) {
            log_error("Did not receive all data.\n\n");
            return 1;
        }

        log_info("Moving tempBuf to mainBuf.");
        memcpy(mainBuf + totalBytesRecevied, tempBuf, bytesReceived);
        totalBytesRecevied += bytesReceived;
        mainBuf[totalBytesRecevied] = '\0';

        // Reallocate buffer if it is half full
        if (strlen(mainBuf) > (size_t)(main_buf_size / 2)) {
            main_buf_size = main_buf_size * 5;
            mainBuf = (char *)realloc(mainBuf, main_buf_size * sizeof(char));
        }

        log_info("Calculating if there is a message.");

        fillRequestResult = fillRequest(mainBuf, totalBytesRecevied, request);
        if (fillRequestResult > 0) {
            return 0;
        } else if (fillRequestResult == -1) {
            return 1;
        }
    }
    return 1;
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
int tcp_server_send_response(int socket, Response response) {

    printf("server: sending response\n");
    if (send(socket, response.message, strlen(response.message), 0) <
        (ssize_t)strlen(response.message)) {
        log_error("Could not send all data.");
        return 1;
    }

    return 0;
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
int tcp_server_client_cleanup(int socket, Request request, Response response) {
    log_info("Freeing request");
    free(request.action);
    free(request.message);

    log_info("Freeing response");
    free(response.message);

    log_info("Closing socket");
    if (socket != 1)
        close(socket);
    return socket;
}

/*
Description:
    Cleans up allocated resources and sockets.
Arguments:
    int socket: The server socket to close.
Return value:
    Returns a 1 on failure, 0 on success.
*/
int tcp_server_cleanup(int socket) { return close(socket); }

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
int tcp_server_process_request(Request request, Response *response) {

    response->message = malloc(strlen(request.message) + 1);
    memcpy(response->message, request.message, strlen(request.message));
    response->message[strlen(request.message)] = '\0';
    log_info("Building Message to be sent.");

    if (strcmp(request.action, "reverse") == 0) {
        log_info("Reverse Message!\n");
        reverse(response->message);
    } else if (strcmp(request.action, "uppercase") == 0) {
        log_info("uppercase Message!\n");
        uppercase(response->message);
    } else if (strcmp(request.action, "lowercase") == 0) {
        log_info("lowercase Message!\n");
        lowercase(response->message);
    } else if (strcmp(request.action, "title-case") == 0) {
        log_info("title-case Message!\n");
        titlecase(response->message);
    } else if (strcmp(request.action, "shuffle") == 0) {
        log_info("shuffle Message!\n");
        shuffle(response->message);
    } else {
        log_info("invalid action tcp_server_build_response");
        free(response->message);
        return -1;
    }

    response->message[strlen(request.message)] = '\0';
    log_info("Message Built!");
    printf("response message: %s\n", response->message);
    return 0;
}
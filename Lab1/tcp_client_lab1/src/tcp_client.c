#include "log.h"

#include "tcp_client.h"

#define NUMVALIDACTIONS 5
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "8080"
#define SERVER_ARGS 2
#define MAX_DATA_SIZE 1024

bool verboseFlag = false;
// comment
char usagePattern[283] = "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n"
                         "\nArguments:\n"
                         "ACTION   Must be uppercase, lowercase, title-case,\n"
                         "\t reverse, or shuffle.\n"
                         "MESSAGE  Message to send to the server\n"
                         "\nOptions:\n"
                         "  --help\n"
                         "  -v, --verbose\n"
                         "  --host HOSTNAME, -h HOSTNAME\n"
                         "  --port PORT, -p PORT\n";

char validActions[NUMVALIDACTIONS][10] = {"uppercase", "lowercase", "title-case", "reverse",
                                          "shuffle"};

/*
Description:
    Print message that is passed in into log_info if user turns on verbose flag.
Arguments:
    char *message: The message to print into log info.
Return value:
    void: Does not have a return value;
*/
void printIfVerbose(char *message) {
    if (verboseFlag == true) {
        log_info("%s", message);
    }
}

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {

    int option;
    bool portSet = 0;
    bool hostSet = 0;
    printIfVerbose("Parsing arguments");
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {{"help", no_argument, 0, '1'},
                                               {"port", required_argument, 0, 'p'},
                                               {"host", required_argument, 0, 'h'},
                                               {"verbose", no_argument, 0, 'v'},
                                               {0, 0, 0, 0}};

        option = getopt_long(argc, argv, ":vp:h:", long_options, &option_index);
        if (option == -1)
            break;

        switch (option) {
        case 1:
            log_info("%s", usagePattern);
            break;
        case 'p':
            config->port = optarg;
            portSet = 1;
            break;
        case 'h':
            config->host = optarg;
            hostSet = 1;
            break;
        case 'v':
            verboseFlag = true;
            break;
        case ':': // Missing option argument
            log_error("Missing option argument\n\n");
            log_info("%s\n\n", usagePattern);
            return 1;
            break;
        case '?': // Invalid option
            log_error("Invalid option\n\n");
            log_info("%s\n\n", usagePattern);
            return 1;
            break;
        default:
            break;
        }
    }

    if ((argc - optind) != SERVER_ARGS) {
        log_error("Invalid argument ammount\n\n");
        log_info("%s\n\n", usagePattern);
        return 1;
    }

    // Make sure Action is valid
    printIfVerbose("Checking if action is valid");
    for (int i = 0; i < NUMVALIDACTIONS; i++) {
        if (strcmp(validActions[i], argv[optind]) == 0) {
            config->action = argv[optind];
            break;
        }
        if (i == NUMVALIDACTIONS - 1) {
            log_error("Invalid Action\n\n");
            log_info("%s\n\n", usagePattern);
            return 1;
        }
    }

    // Assign message
    config->message = argv[++optind];

    // If port and host were not specified, set to default

    if (portSet == 0) {
        printIfVerbose("Setting default port: \"8080\"");
        config->port = DEFAULT_PORT;
    }
    if (hostSet == 0) {
        printIfVerbose("Setting defualt host: \"localhost\" (127.0.0.1)");
        config->host = DEFAULT_HOST;
    }
    return 0;
}

/*
Description:
    Creates a TCP socket and connects it to the specified host and port.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_client_connect(Config config) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(config.host, config.port, &hints, &servinfo)) != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    printIfVerbose("Finding socket");
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            log_error("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            log_error("client: connect");
            continue;
        } else {
            // If it gets here, if found a socket to connect to.
            printIfVerbose("Connected to socket");
            freeaddrinfo(servinfo);
            printIfVerbose("Freed servinfo variable");
            return sockfd;
        }

        break;
    }

    // It didn't connect so just return -1;
    log_error("Could not find a socket");
    return -1;
}

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    Config config: A config struct with the necessary information.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, Config config) {
    char fullMessage[MAX_DATA_SIZE]; // String to append full message to.

    // Put together fullMessage to send to the server.
    printIfVerbose("Building message");
    sprintf(fullMessage, "%s %ld %s", config.action, strlen(config.message), config.message);

    printIfVerbose("Sending message");
    if (send(sockfd, fullMessage, strlen(fullMessage), 0) < (ssize_t)strlen(fullMessage)) {
        log_error("Could not send all data.\n");
        return 1;
    }
    return 0;
}

/*
Description:
    Receives the response from the server. The caller must provide an already allocated buffer.
Arguments:
    int sockfd: Socket file descriptor
    char *buf: An already allocated buffer to receive the response in
    int buf_size: The size of the allocated buffer
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, char *buf, int buf_size) {
    int numbytes;

    printIfVerbose("Receiving message");
    if ((numbytes = recv(sockfd, buf, buf_size - 1, 0)) == -1) {
        log_error("Did not receive all data.\n\n");
        return 1;
    }

    // Add null terminator to end of string so that it can be easily used.
    buf[numbytes] = '\0';
    return 0;
}

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd) {
    printIfVerbose("Closing socket");
    return close(sockfd);
}
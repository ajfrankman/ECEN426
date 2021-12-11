#include "log.h"

#include "tcp_client.h"
#include <ctype.h>

#define NUMVALIDACTIONS 5
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "8082"
#define SERVER_ARGS 1
#define ACTION_LENGTH_BYTES 4
#define MAX_ACTION_LENGTH 20

#define UPPERCASE 0x01
#define LOWERCASE 0X02
#define TITLECASE 0x04
#define REVERSE 0x08
#define SHUFFLE 0x10

const char validActions[NUMVALIDACTIONS][10] = {"uppercase", "lowercase", "title-case", "reverse",
                                                "shuffle"};

bool verboseFlag = false;

char usagePattern[283] = "\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] FILE\n"
                         "\nArguments:\n"
                         "  FILE   A file name containing actions and messages to\n"
                         "send to the server. If \"-\" is provided, stdin will\n"
                         "be read.\n"
                         "\nOptions:\n"
                         "  --help\n"
                         "  -v, --verbose\n"
                         "  --host HOSTNAME, -h HOSTNAME\n"
                         "  --port PORT, -p PORT\n";

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

    // Assign file
    config->file = argv[optind];

    // If port and host were not specified, set to default
    if (portSet == 0) {
        printIfVerbose("Setting default port: \"8081\"");
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
    Given a action and message strings return the binary representation of that action
    if it exists as well as the message length.
Arguments:
    char *action: The string containing the action.
Return value:
    Returns binary representation of action and message length.
    Returns 100 if the action isn't valid.
*/
uint32_t getActionAndLength(char *action, char *message) {
    uint32_t actionAndLength;
    if (strcmp("uppercase", action) == 0) {
        actionAndLength = UPPERCASE;
    } else if (strcmp("lowercase", action) == 0) {
        actionAndLength = LOWERCASE;
    } else if (strcmp("title-case", action) == 0) {
        actionAndLength = TITLECASE;
    } else if (strcmp("reverse", action) == 0) {
        actionAndLength = REVERSE;
    } else if (strcmp("shuffle", action) == 0) {
        actionAndLength = SHUFFLE;
    } else {
        log_error("Invalid Action received.\n");
        return 100;
    }

    actionAndLength = actionAndLength << 27;
    actionAndLength = actionAndLength | ((uint32_t)strlen(message));
    return actionAndLength;
}

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    char *action: The action that will be sent
    char *message: The message that will be sent
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, char *action, char *message) {
    uint32_t actionAndLength = getActionAndLength(action, message);
    uint32_t networkActionAndLength = htonl(actionAndLength);

    if (send(sockfd, &networkActionAndLength, 4, 0) < 4) {
        log_error("Could not send protocol header data.\n");
        return 1;
    }
    if (send(sockfd, message, strlen(message), 0) < (ssize_t)strlen(message)) {
        log_error("Could not send all data.\n");
        return 1;
    }

    return 0;
}

/*
Description:
    Get the length of the first buffer before the actually message.
Arguments:
    char *message: The current input from the server.
Return value:
    Returns length of header (number and space)
*/
int getHeaderLength(char *message) {
    int length = atoi(message);
    char lengthBuf[10];
    sprintf(lengthBuf, "%d", length);
    return 1 + strlen(lengthBuf);
}

/*
Description:
    Returns length of full message including header number if it exists in buffer.
Arguments:
    char *message: The current input from the server.
    int totalBytes: total byts in message.
Return value:
    Returns a message length including header length on success, and 0 if a full message doesn't
exist.
*/
int checkForMessage(char *message, uint32_t totalBytes) {
    // Check for number in first element.
    if (totalBytes >= 4) {
        // Read the number in.
        printIfVerbose("Greater than 4 bytes -> checking for message of correct length.");

        uint32_t messageLength = 0;
        messageLength = (int)(message[0] << 24 | message[1] << 16 | message[2] << 8 | message[3]);
        // Check if there are enough characters after.
        if (totalBytes >= (ACTION_LENGTH_BYTES + messageLength)) {
            // If message exists return messagelength including header.
            return messageLength + ACTION_LENGTH_BYTES;
        }
        return 0;
    }
    return 0;
}

/*
Description:
    Receives the response from the server. The caller must provide a function pointer that handles
the response and returns a true value if all responses have been handled, otherwise it returns a
    false value. After the response is handled by the handle_response function pointer, the response
    data can be safely deleted. The string passed to the function pointer must be null terminated.
Arguments:
    int sockfd: Socket file descriptor
    int (*handle_response)(char *): A callback function that handles a response
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *)) {

    int totalBytesRecevied = 0;
    int bytesReceived = 0;
    int main_buf_size = 500;
    char *mainBuf;
    mainBuf = (char *)malloc(main_buf_size * sizeof(char));
    int temp_buf_size = 500;
    char *tempBuf;
    tempBuf = (char *)malloc(temp_buf_size * sizeof(char));
    int movePointer;

    // Keep getting data until we receive full message;
    printIfVerbose("Starting to get response.");
    while (true) {
        if ((bytesReceived = recv(sockfd, tempBuf, main_buf_size - totalBytesRecevied, 0)) == -1) {
            log_error("Did not receive all data.\n\n");
            return 1;
        }
        printIfVerbose("Moving tempBuf to mainBuf.");
        memcpy(mainBuf + totalBytesRecevied, tempBuf, bytesReceived);
        totalBytesRecevied += bytesReceived;
        mainBuf[totalBytesRecevied] = '\0';
        // udate total bytes received.
        if (strlen(mainBuf) > (size_t)(main_buf_size / 2)) {
            main_buf_size = main_buf_size * 5;
            mainBuf = (char *)realloc(mainBuf, main_buf_size * sizeof(char));
        }
        printIfVerbose("Calculating if there is a message.");
        while ((movePointer = checkForMessage(mainBuf, totalBytesRecevied)) > 0) {
            printIfVerbose("Found a message. Now Building.");
            char *handleString;
            handleString = (char *)malloc((movePointer - 3) * sizeof(char));
            int headerLength = ACTION_LENGTH_BYTES;
            int messageLength = movePointer - headerLength;
            memcpy(handleString, mainBuf + headerLength, messageLength);
            handleString[messageLength] = '\0';

            // Call handle. If true we are done.
            printIfVerbose("Calling handle_response().");
            if (handle_response(handleString) == true) {
                free(handleString);
                free(mainBuf);
                free(tempBuf);
                return 0;
            }

            printIfVerbose("Moving buffer for safe memory stuff. :)");
            memmove(mainBuf, mainBuf + movePointer, totalBytesRecevied - movePointer);
            totalBytesRecevied -= movePointer;
            free(handleString);
        }
    }
    printIfVerbose("No Message to Receive.");
    return 1;
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

/*
Description:
    Opens a file.
Arguments:
    char *file_name: The name of the file to open
Return value:
    Returns NULL on failure, a FILE pointer on success
*/
FILE *tcp_client_open_file(char *file_name) {
    FILE *myFile;
    printIfVerbose("Opening File.");

    if (file_name[0] == '-') {
        return stdin;
    }
    if ((myFile = fopen(file_name, "r")) == NULL) {
        log_error("Could not open file.");
        exit(EXIT_FAILURE);
    }
    printIfVerbose("File opened.");
    return myFile;
}

/*
Description:
    Gets the next line of a file, filling in action and message. This function should be similar
    design to getline() (https://linux.die.net/man/3/getline). *action and message must be
    allocated by the function and freed by the caller.* When this function is called, action must
    point to the action string and the message must point to the message string.
Arguments:
    FILE *fd: The file pointer to read from
    char **action: A pointer to the action that was read in
    char **message: A pointer to the message that was read in
Return value:
    Returns -1 on failure, the number of characters read on success
*/
int tcp_client_get_line(FILE *fd, char **action, char **message) {
    ssize_t read;
    char *line = NULL;
    size_t len = 0;

    printIfVerbose("Getting File Input Line.");
    while ((read = getline(&line, &len, fd)) != -1) {
        *action = malloc(MAX_ACTION_LENGTH);
        printIfVerbose("Parsing Action.");

        *message = malloc(read);
        printIfVerbose("Parsing Message.");

        if (sscanf(line, "%s %[^\n]", *action, *message) != 2) {
            free(*action);
            *action = NULL;
            free(*message);
            *message = NULL;
            free(line);
            return -1;
        }

        free(line);
        return read;
    }
    free(line);
    return -1;
}

/*
Description:
    Closes a file.
Arguments:
    FILE *fd: The file pointer to close
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close_file(FILE *fd) {
    printIfVerbose("Closing File.");
    return fclose(fd);
}
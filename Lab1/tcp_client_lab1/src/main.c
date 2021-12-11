// #include <getopt.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// #include "log.h"
// #include "tcp_client.h"

// #define MAX_DATA_SIZE 1024

// void printConfig(Config *config) {
//     printf("config action: %s\n", config->action);
//     printf("config host: %s\n", config->host);
//     printf("config message: %s\n", config->message);
//     printf("config port: %s\n", config->port);
// }

// int main(int argc, char *argv[]) {
//     struct Config config; // This will store the command data.
//     int socket;           // These are to check the return values of the different functions.

//     if (tcp_client_parse_arguments(argc, argv, &config) == 1) {
//         log_error("tcp_client_parse() returned an error code.\n");
//         return EXIT_FAILURE;
//     }

//     // Connect to a socket and save it's value;
//     if ((socket = tcp_client_connect(config)) == -1) { // Error checking
//         log_error("tcp_client_connect() returned an error code.\n");
//         return EXIT_FAILURE;
//     }

//     if (tcp_client_send_request(socket, config) == 1) {
//         log_error("tcp_client_send_request() returned an error code.\n");
//         return EXIT_FAILURE;
//     }

//     char responseMessage[strlen(config.message)]; // Allocate space for the repsones message.
//     if (tcp_client_receive_response(socket, responseMessage, strlen(config.message) + 1) == 1) {
//         log_error("tcp_clien_receive_response() returned an error code.\n");
//         return EXIT_FAILURE;
//     }

//     printf("%s\n", responseMessage);
//     if (tcp_client_close(socket) == 1) {
//         log_error("tcp_client_close() returned an error code.\n");
//         return EXIT_FAILURE;
//     }

//     return EXIT_SUCCESS;
// }

main() {
    char string = "Hello World";

    printf("%s", string);
}
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "tcp_client.h"

#define MAX_DATA_SIZE 1024
#define NUM_SPACES 2

int numRequests = 0;

int handle_response(char *string) {
    numRequests--;
    printf("%s\n", string);
    if (numRequests == 0)
        return true;
    return false;
}

int main(int argc, char *argv[]) {
    Config config;
    int socket;
    char *action;
    char *message;

    if (tcp_client_parse_arguments(argc, argv, &config) == 1) {
        log_error("Something went wrong in tcp_client_parse_arg()");
        exit(EXIT_FAILURE);
    }
    if ((socket = tcp_client_connect(config)) == -1) {
        log_error("Could not connect to Server.");
        exit(EXIT_FAILURE);
    }

    FILE *myFile;

    myFile = tcp_client_open_file(config.file);

    while (tcp_client_get_line(myFile, &action, &message) > 0) {
        if (tcp_client_send_request(socket, action, message) == 1) {
            log_error("Could not send action \"%s\" and message \"%s\"\n", action, message);
            tcp_client_close_file(myFile);
            exit(EXIT_FAILURE);
        }
        numRequests++;
    }
    tcp_client_close_file(myFile);
    tcp_client_receive_response(socket, handle_response);
    tcp_client_close(socket);
    exit(EXIT_SUCCESS);
}
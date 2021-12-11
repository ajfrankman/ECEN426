#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "log.h"
#include "tcp_server.h"

int theirSocket;
int mySocket;

Request request;
Response response;

bool volatile keepRunning = 1;

void intHandler() {

    log_info("Caught ctrl-c. Cleaning...");
    if (theirSocket == -1) {
        tcp_server_client_cleanup(theirSocket, request, response);
    }
    tcp_server_cleanup(mySocket);
    keepRunning = 0;

    log_error("Exitting because of ctrl-c");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, intHandler);

    Config config;

    if (tcp_server_parse_arguments(argc, argv, &config) == 1) {
        log_error("Could not parse arguments.");
        return 0;
    }

    if ((mySocket = tcp_server_create(config)) == -1) {
        log_error("Socket: %d. Could not create.");
        return 0;
    }

    while (keepRunning) { // main accept() loop
        if ((theirSocket = tcp_server_accept(mySocket)) == -1) {
            continue;
        }

        request.action = NULL;
        request.message = NULL;
        response.message = NULL;
        if (tcp_server_receive_request(theirSocket, &request) == 1) {
            tcp_server_client_cleanup(theirSocket, request, response);
            continue;
        }
        if (tcp_server_process_request(request, &response) == 1) {
            log_error("Could not build Response message");
            tcp_server_client_cleanup(theirSocket, request, response);
            continue;
        }
        tcp_server_send_response(theirSocket, response);
        tcp_server_client_cleanup(theirSocket, request, response);
    }
    return 0;
}

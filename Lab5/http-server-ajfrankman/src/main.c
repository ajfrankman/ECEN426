#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "http_server.h"
#include "log.h"

int theirSocket;
int mySocket;

Header header;
Request request;
Response response;
Config config;

int main(int argc, char *argv[]) {

    http_server_parse_arguments(argc, argv, &config);

    if ((mySocket = http_server_create(config)) == -1) {
        log_error("Could not create socket.");
        return 0;
    }

    while (true) { // main accept() loop
        if ((theirSocket = http_server_accept(mySocket)) == -1) {
            continue;
        }
        if (http_server_receive_request(theirSocket, &request) == 1) {
            log_error("Receive Error. Cleaning up...");
            // http_server_client_cleanup(theirSocket, request, response);

            continue;
        }
        printf("config.relative_path: %s\n", config.relative_path);
        printf("request path: %s\n", request.path);
        if (http_server_process_request(request, config.relative_path, &response) == 1) {
            log_error("Could not build Response.");
            // http_server_client_cleanup(theirSocket, request, response);
            continue;
        }

        if (http_server_send_response(theirSocket, response) == 1) {
            log_error("Could not send response");
            continue;
        }

        http_server_client_cleanup(theirSocket, request, response);
    }
    http_server_cleanup(mySocket);
}
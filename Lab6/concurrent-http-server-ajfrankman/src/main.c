#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "http_server.h"
#include "log.h"

Config config;

int thread_count;
pthread_t *threads;
int mySocket;
bool running = true;

void intHandler() {

    log_info("Caught ctrl-c. Waiting for responses to finish...");
    running = false;
    http_server_cleanup(mySocket);
}

pthread_t *add_thread(pthread_t *threads[], int *thread_count, int *thread_size) {
    // If we don't have room, add more space in the threads array
    if (*thread_count == *thread_size) {
        *thread_size *= 2; // Double it

        *threads = realloc(*threads, sizeof(**threads) * (*thread_size));
    }

    pthread_t *new_thread = &(*threads)[*thread_count];
    (*thread_count)++;
    return new_thread;
}

void *handle_client(void *arg) {
    int clientSocket = *(int *)arg;
    Request request;
    Response response;

    if (http_server_receive_request(clientSocket, &request) == 1) {
        log_error("Receive Error. Cleaning up...");
        http_server_client_cleanup(clientSocket, request, response);
        return (void *)0;
    }
    sleep(5);
    if (http_server_process_request(request, config.relative_path, &response) == 1) {
        log_error("Could not build Response.");
        http_server_client_cleanup(clientSocket, request, response);
        return (void *)0;
    }
    if (http_server_send_response(clientSocket, response) == 1) {
        log_error("Could not send response");
        http_server_client_cleanup(clientSocket, request, response);
        return (void *)0;
    }
    printf("server: response sent\n");

    http_server_client_cleanup(clientSocket, request, response);
    free(clientSocket);
    return (void *)0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);

    thread_count = 0;
    int thread_size = 5;
    threads = malloc(sizeof *threads * thread_size);

    http_server_parse_arguments(argc, argv, &config);

    if ((mySocket = http_server_create(config)) == -1) {
        log_error("Could not create socket.");
        return 0;
    }

    while (running) { // main accept() loop
        int *sock = malloc(sizeof(int));
        if ((*sock = http_server_accept(mySocket)) == -1) {
            continue;
        }

        pthread_t *aThread = add_thread(&threads, &thread_count, &thread_size);
        pthread_create(aThread, NULL, handle_client, (void *)sock);
        printf("thread_count main: %d\n", thread_count);
    }

    for (int i = 0; i < thread_count; i++) {
        printf("trying to join\n");
        pthread_join(threads[i], NULL);
        printf("joined!\n");
    }

    threads = NULL;
    free(threads);
    log_info("Responses done. Bye!");

    return EXIT_SUCCESS;
}
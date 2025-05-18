#include "analyzer.h"
#include "message.h"
#include "fragmentation.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/mapd_socket"

extern void* fragmentation_thread(void*);
static int client_counter = 0;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;


void* handle_client(void* arg) {
    ClientContext* ctx = (ClientContext*)arg;
    char buffer[1024];
    ssize_t bytes_read;

    printf("[Analyzer] New connection: Client #%d (fd = %d)\n",
           ctx->client_number, ctx->client_fd);

    while ((bytes_read = read(ctx->client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';

        char* line = strtok(buffer, "\n");
        while (line != NULL) {
            if (strlen(line) == 0 || line[0] != '{') {
                line = strtok(NULL, "\n");
                continue;
            }

            Message msg = parse_json_to_message(line, ctx->client_number);
            enqueue_message(&msg);
            line = strtok(NULL, "\n");
        }
    }


    printf("[Analyzer] Client #%d disconnected.\n", ctx->client_number);
    close(ctx->client_fd);
    free(ctx);
    return NULL;
}


void* gui_consumer_thread(void* arg) {
    (void)arg; // unused
    while (1) {
        Message msg = dequeue_message();
        printf("[GUI] Client %d | %-12s | Addr: %-12s | Size: %-5zu | Thread: %lu | Time: %ld\n",
               msg.client_id, msg.type, msg.addr, msg.size, msg.thread, msg.timestamp);
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_un addr;

    pthread_t gui_thread;
    pthread_create(&gui_thread, NULL, gui_consumer_thread, NULL);
    pthread_detach(gui_thread);

    pthread_t frag_thread;
    pthread_create(&frag_thread, NULL, fragmentation_thread, NULL);
    pthread_detach(frag_thread);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 10);
    printf("[Analyzer] Listening on %s\n", SOCKET_PATH);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        ClientContext* ctx = malloc(sizeof(ClientContext));
        if (!ctx) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        ctx->client_fd = client_fd;
        pthread_mutex_lock(&counter_lock);
        ctx->client_number = ++client_counter;
        pthread_mutex_unlock(&counter_lock);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, ctx) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(ctx);
        } else {
            ctx->thread_id = tid;
            pthread_detach(tid);
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
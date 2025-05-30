#ifndef ANALYZER_H
#define ANALYZER_H

#include <pthread.h>

typedef struct {
    int client_fd;
    pthread_t thread_id;
    int client_number;
} ClientContext;

void* handle_client(void* arg);

void* gui_consumer_thread(void* arg);

#endif

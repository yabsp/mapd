#ifndef ANALYZER_H
#define ANALYZER_H

#include <pthread.h>
#include "message.h"
#include "fragmentation.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/**
 * AnalyzerOptions:
 *
 * Configuration struct for the analyzer module. Holds thresholds and flags used by the analyzer and fragmentation.
 */
typedef struct {
    int small_threshold;
    int large_threshold;
    int info_logs_enabled;
} AnalyzerOptions;

/**
 * ClientContext:
 *
 * Per-client information used by handle_client() to manage an active connection.
 */
typedef struct {
    int client_fd;
    pthread_t thread_id;
    int client_number;
} ClientContext;

/**
 * Global pointer to analyzer configuration options.
 */
extern AnalyzerOptions* analyzer_options;

/**
 * analyzer_init:
 *
 * Starts the analyzer service: spawns the fragmentation monitoring thread and starts listening for incoming clients.
 *
 * @param options Pointer to AnalyzerOptions struct
 */
void analyzer_init(AnalyzerOptions* options);

/**
 * handle_client:
 *
 * Worker thread function to handle one client connection.
 *
 * @param arg Pointer to ClientContext struct
 * @return Always NULL
 */
void* handle_client(void* arg);

/**
 * gui_consumer_thread:
 *
 * Background thread which consumes messages from the queue and prints logs for the GUI.
 *
 * @param arg Unused
 * @return Always NULL
 */
void* gui_consumer_thread(const void* arg);
#endif

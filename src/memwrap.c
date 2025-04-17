#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>

#include "memwrap.h"

#define SOCKET_PATH "/tmp/mapd_socket"

static int sock_fd = -1;
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;

const char* event_type_to_string(EventType type) {
    switch (type) {
        case EVENT_MALLOC: return "malloc";
        case EVENT_FREE: return "free";
        case EVENT_MEMORY_LEAK: return "memory_leak";
        case EVENT_DANGLING_POINTER: return "dangling_pointer";
        case EVENT_BUFFER_OVERFLOW: return "buffer_overflow";
        case EVENT_DOUBLE_FREE: return "double_free";
        default: return "unknown";
    }
}

__attribute__((constructor)) // runs before main() starts in target
void setup_connection() {
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("[Wrapper] socket");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("[Wrapper] connect");
        close(sock_fd);
        sock_fd = -1;
    } else {
        fprintf(stderr, "[Wrapper] Connected to analyzer.\n");
    }
}

void send_json_event(EventType type, void* addr, size_t size) {
    if (sock_fd == -1) return;

    char msg[512];
    snprintf(msg, sizeof(msg),
        "{ \"type\": \"%s\", \"addr\": \"%p\", \"size\": %zu, \"thread\": %lu, \"timestamp\": %ld }\n",
        event_type_to_string(type),
        addr,
        size,
        (unsigned long)pthread_self(),
        time(NULL)
    );

    write(sock_fd, msg, strlen(msg));
}

void* malloc(size_t size) {
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }

    void* ptr = real_malloc(size);
    if (ptr) {
        send_json_event(EVENT_MALLOC, ptr, size);
    }

    return ptr;
}

void free(void* ptr) {
    if (!real_free) {
        real_free = dlsym(RTLD_NEXT, "free");
    }

    if (ptr) {
        send_json_event(EVENT_FREE, ptr, 0);
    }

    real_free(ptr);
}

__attribute__((destructor))  // after main() in target program returns
void shutdown_connection() {
    if (sock_fd != -1) {
        close(sock_fd);
        fprintf(stderr, "[Wrapper] Disconnected from analyzer.\n");
    }
}

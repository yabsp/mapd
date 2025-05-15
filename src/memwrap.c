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
#include <execinfo.h>  // for optional backtrace debugging

#include "memwrap.h"

#define SOCKET_PATH "/tmp/mapd_socket"
#define MAX_TRACKED_ALLOCS 10000

static int sock_fd = -1;
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static int tracking_enabled = 0;

typedef struct {
    void* addr;
    size_t size;
} AllocationEntry;

static AllocationEntry allocations[MAX_TRACKED_ALLOCS];
static int allocation_count = 0;
static pthread_mutex_t allocation_lock = PTHREAD_MUTEX_INITIALIZER;

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

// Public function for test_alloc to call
void enable_tracking() {
    tracking_enabled = 1;
}

__attribute__((constructor))
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

    if (tracking_enabled && ptr) {
        pthread_mutex_lock(&allocation_lock);
        if (allocation_count < MAX_TRACKED_ALLOCS) {
            allocations[allocation_count++] = (AllocationEntry){ ptr, size };
        }
        pthread_mutex_unlock(&allocation_lock);

        send_json_event(EVENT_MALLOC, ptr, size);
    }

    return ptr;
}

void free(void* ptr) {
    if (!real_free) {
        real_free = dlsym(RTLD_NEXT, "free");
    }

    if (tracking_enabled && ptr) {
        int found = 0;
        size_t freed_size = 0;

        pthread_mutex_lock(&allocation_lock);
        for (int i = 0; i < allocation_count; ++i) {
            if (allocations[i].addr == ptr) {
                freed_size = allocations[i].size;
                allocations[i] = allocations[--allocation_count];
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&allocation_lock);

        if (found) {
            send_json_event(EVENT_FREE, ptr, freed_size);
            real_free(ptr);  // ✅ Only free real memory if this is a valid first-time free
        } else {
            send_json_event(EVENT_DOUBLE_FREE, ptr, 0);
            // ❌ Do not free again to avoid crash
            return;
        }
    } else {
        real_free(ptr);  // fallback: still free if tracking is off
    }
}




__attribute__((destructor))
void shutdown_connection() {
    if (tracking_enabled) {
        pthread_mutex_lock(&allocation_lock);
        for (int i = 0; i < allocation_count; ++i) {
            send_json_event(EVENT_MEMORY_LEAK, allocations[i].addr, allocations[i].size);
        }
        pthread_mutex_unlock(&allocation_lock);
    }

    if (sock_fd != -1) {
        close(sock_fd);
        fprintf(stderr, "[Wrapper] Disconnected from analyzer.\n");
    }
}

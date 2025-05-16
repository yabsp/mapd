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
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <execinfo.h>
#include "memwrap.h"

#define SOCKET_PATH "/tmp/mapd_socket"
#define MAX_TRACKED_ALLOCS 10000
#define MAX_FREED_REGIONS 10000

static int sock_fd = -1;
static void* (*real_malloc)(size_t) = NULL;
static void* (*real_free)(void*) = NULL;
static int tracking_enabled = 0;
static volatile sig_atomic_t crashed = 0;

typedef struct {
    void* addr;
    size_t requested_size;
    size_t allocated_size;
} AllocationEntry;

static AllocationEntry allocations[MAX_TRACKED_ALLOCS];
static int allocation_count = 0;
static pthread_mutex_t allocation_lock = PTHREAD_MUTEX_INITIALIZER;

static AllocationEntry freed_regions[MAX_FREED_REGIONS];
static int freed_region_count = 0;
static pthread_mutex_t freed_lock = PTHREAD_MUTEX_INITIALIZER;

const char* event_type_to_string(EventType type) {
    switch (type) {
        case EVENT_MALLOC: return "malloc";
        case EVENT_FREE: return "free";
        case EVENT_MEMORY_LEAK: return "memory_leak";
        case EVENT_DANGLING_POINTER: return "dangling_pointer";
        case EVENT_BUFFER_OVERFLOW: return "buffer_overflow";
        case EVENT_DOUBLE_FREE: return "double_free";
        case EVENT_FORCED_CRASH: return "forced_crash";
        default: return "unknown";
    }
}

void send_json_event(EventType type, void* addr, size_t size) {
    if (sock_fd == -1) return;
    char msg[512];
    snprintf(msg, sizeof(msg),
        "{ \"type\": \"%s\", \"addr\": \"%p\", \"size\": %zu, \"thread\": %lu, \"timestamp\": %ld }\n",
        event_type_to_string(type), addr, size,
        (unsigned long)pthread_self(), time(NULL));
    write(sock_fd, msg, strlen(msg));
}

void detect_memory_leaks() {
    pthread_mutex_lock(&allocation_lock);
    for (int i = 0; i < allocation_count; ++i) {
        send_json_event(EVENT_MEMORY_LEAK, allocations[i].addr, allocations[i].requested_size);
    }
    pthread_mutex_unlock(&allocation_lock);
}

void handle_segv(int sig, siginfo_t* info, void* context) {
    void* fault_addr = info->si_addr;

    // Check for dangling pointer
    pthread_mutex_lock(&freed_lock);
    for (int i = 0; i < freed_region_count; ++i) {
        void* start = freed_regions[i].addr;
        void* end = (void*)((uintptr_t)start + freed_regions[i].allocated_size);
        if (fault_addr >= start && fault_addr < end) {
            send_json_event(EVENT_DANGLING_POINTER, start, freed_regions[i].requested_size);
            send_json_event(EVENT_FORCED_CRASH, start, freed_regions[i].requested_size);
            munmap(start, freed_regions[i].allocated_size);
            freed_regions[i] = freed_regions[--freed_region_count];
            goto exit_crash;
        }
    }
    pthread_mutex_unlock(&freed_lock);

    // Check for buffer overflow
    pthread_mutex_lock(&allocation_lock);
    size_t pagesize = sysconf(_SC_PAGESIZE);
    for (int i = 0; i < allocation_count; ++i) {
        void* guard = (void*)((uintptr_t)allocations[i].addr + allocations[i].allocated_size - pagesize);
        void* end = (void*)((uintptr_t)guard + pagesize);
        if (fault_addr >= guard && fault_addr < end) {
            send_json_event(EVENT_BUFFER_OVERFLOW, allocations[i].addr, allocations[i].requested_size);
            send_json_event(EVENT_FORCED_CRASH, allocations[i].addr, allocations[i].requested_size);
            break;
        }
    }
    pthread_mutex_unlock(&allocation_lock);

exit_crash:
    fprintf(stderr, "[Wrapper] Error: Crashing due to memory violation.\n");
    fsync(sock_fd);
    usleep(10000);
    if (sock_fd != -1) close(sock_fd);
    crashed = 1;
    exit(EXIT_FAILURE);
}

void enable_tracking() {
    tracking_enabled = 1;
}

__attribute__((constructor))
void setup_connection() {
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock_fd);
        sock_fd = -1;
    } else {
        fprintf(stderr, "[Wrapper] Connected to analyzer.\n");
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_segv;
    sigaction(SIGSEGV, &sa, NULL);
}

void* malloc(size_t size) {
    if (!tracking_enabled) {
        if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
        return real_malloc(size);
    }

    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t usable = ((size + pagesize - 1) / pagesize) * pagesize;
    size_t total = usable + pagesize;

    void* base = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return NULL;

    void* guard = (void*)((uintptr_t)base + usable);
    mprotect(guard, pagesize, PROT_NONE);

    pthread_mutex_lock(&allocation_lock);
    if (allocation_count < MAX_TRACKED_ALLOCS) {
        allocations[allocation_count++] = (AllocationEntry){
            .addr = base,
            .requested_size = size,
            .allocated_size = total
        };
    }
    pthread_mutex_unlock(&allocation_lock);

    send_json_event(EVENT_MALLOC, base, size);
    return base;
}

void free(void* ptr) {
    if (!tracking_enabled || ptr == NULL) {
        if (!real_free) real_free = dlsym(RTLD_NEXT, "free");
        real_free(ptr);
        return;
    }

    int found = 0;
    size_t requested = 0, alloc_size = 0;

    pthread_mutex_lock(&allocation_lock);
    for (int i = 0; i < allocation_count; ++i) {
        if (allocations[i].addr == ptr) {
            requested = allocations[i].requested_size;
            alloc_size = allocations[i].allocated_size;
            allocations[i] = allocations[--allocation_count];
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&allocation_lock);

    if (!found) {
        send_json_event(EVENT_DOUBLE_FREE, ptr, 0);
        return;
    }

    send_json_event(EVENT_FREE, ptr, requested);

    if (mprotect(ptr, alloc_size, PROT_NONE) == 0) {
        pthread_mutex_lock(&freed_lock);
        if (freed_region_count < MAX_FREED_REGIONS) {
            freed_regions[freed_region_count++] = (AllocationEntry){
                .addr = ptr,
                .requested_size = requested,
                .allocated_size = alloc_size
            };
        }
        pthread_mutex_unlock(&freed_lock);
        return;
    }

    munmap(ptr, alloc_size);  // fallback
}

__attribute__((destructor))
void shutdown_connection() {
    if (tracking_enabled && !crashed) {
        detect_memory_leaks();
    }
    if (sock_fd != -1) {
        close(sock_fd);
        fprintf(stderr, "[Wrapper] Disconnected from analyzer.\n");
    }
}

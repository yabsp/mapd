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
#include "memwrap.h"

#define SOCKET_PATH "/tmp/mapd_socket"
#define MAX_TRACKED_ALLOCS 10000
#define MAX_FREED_REGIONS 10000
#define GUARD_THRESHOLD 1024

/**
 * @file memwrap.c
 * @brief LD_PRELOAD-based memory wrapper to detect leaks, overflows, and dangling pointers.
 *
 * Tracks malloc/free activity via mmap/mprotect, logs events over a UNIX socket,
 * and installs a signal handler for runtime crash detection.
 * Supports runtime modes via MAPD_MODE (debug, test, perf).
 */

// Runtime modes
enum MAPDMode { MODE_DEBUG, MODE_TEST, MODE_PERF };
static enum MAPDMode current_mode = MODE_DEBUG;

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

/**
 * @brief Get the hashed value of a ptr. Used to hash address pointers.
 * malloc is 16 bytes aligned on 64-bit systems -> last 4 bits will most likely be zero.
 * Therefore, shift right 4 bits.
 *
 * @param ptr Pointer of the address.
 * @return Hashed value of the pointer
 */
unsigned long hash_ptr(void* ptr) {
    return ((uintptr_t)ptr >> 4) % MAX_TRACKED_ALLOCS;
}

/**
 * @brief Send a memory event as newline-delimited JSON over the UNIX socket.
 *
 * @param type Event type (e.g., malloc, free, overflow).
 * @param addr Pointer associated with the event.
 * @param size Size of the memory involved (if relevant).
 */


void send_json_event(const EventType type, void* addr, const size_t size)
{
    if (sock_fd == -1 || current_mode == MODE_PERF) return;
    char msg[512];
    snprintf(msg, sizeof(msg),
        "{ \"type\": \"%s\", \"addr\": \"%p\", \"size\": %zu, \"thread\": %lu, \"timestamp\": %ld }\n",
        event_type_to_string(type), addr, size,
    (unsigned long)pthread_self(), time(NULL));
    // only write to socket if no malloc or free -> reduces I/O
    if (current_mode == MODE_DEBUG)
    {
        write(sock_fd, msg, strlen(msg));
    } else if (type != EVENT_FREE && type != EVENT_MALLOC)
    {
        write(sock_fd, msg, strlen(msg));
    }

}

/**
 * @brief Scan the allocation table and report unfreed memory blocks.
 *
 * This function iterates through all entries in the allocation hash table
 * and sends a `memory_leak` event for each entry that was allocated but
 * not freed. It is typically called during program shutdown (via destructor)
 * to detect memory leaks.
 *
 * Thread-safe: acquires the allocation lock to ensure consistent view of the table.
 */
void detect_memory_leaks() {
    pthread_mutex_lock(&allocation_lock);
    for (int i = 0; i < MAX_TRACKED_ALLOCS; ++i) {
        if (allocations[i].addr != NULL)
        {
            send_json_event(EVENT_MEMORY_LEAK, allocations[i].addr, allocations[i].requested_size);
        }
    }
    pthread_mutex_unlock(&allocation_lock);
}

/**
 * @brief SIGSEGV handler to detect and report dangling pointer or buffer overflow errors.
 *
 * Checks faulting address against known freed and allocated regions,
 * sends appropriate events, and terminates the process cleanly.
 */


void handle_segv(int sig __attribute__((unused)), siginfo_t* info, void* context __attribute__((unused))) {
    const void* fault_addr = info->si_addr;

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
    for (int i = 0; i < MAX_TRACKED_ALLOCS; i++) {
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

/**
 * @brief Constructor function that runs before main().
 *
 * Establishes the UNIX socket connection to the analyzer and installs
 * the custom SIGSEGV handler. Also parses the MAPD_MODE environment
 * variable to set the runtime behaviour of the wrapper.
 */

__attribute__((constructor))
void setup_connection() {
    const char* mode_env = getenv("MAPD_MODE");
    if (mode_env) {
        if (strcmp(mode_env, "test") == 0) current_mode = MODE_TEST;
        else if (strcmp(mode_env, "perf") == 0) current_mode = MODE_PERF;
        else current_mode = MODE_DEBUG;
    }
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) return;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock_fd);
        sock_fd = -1;
    } else {
        fprintf(stderr, "[Wrapper] Connected to analyzer.\n");
    }

    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_segv;
    sigaction(SIGSEGV, &sa, NULL);
}

/**
 * @brief Replacement for malloc(), using mmap and optional guard pages.
 *
 * Applies runtime mode logic: falls back to real malloc in perf mode.
 * Otherwise, uses mmap (with guard page depending on alloc size) for overflow detection.
 */

void* malloc(size_t size) {
    if (!tracking_enabled || current_mode == MODE_PERF) {
        if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
        return real_malloc(size);
    }

    const size_t pagesize = sysconf(_SC_PAGESIZE);
    const size_t usable = ((size + pagesize - 1) / pagesize) * pagesize;
    const size_t total = usable + pagesize;

    void* base = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return NULL;

    if (size >= GUARD_THRESHOLD)
    {
        void* guard = (void*)((uintptr_t)base + usable);
        mprotect(guard, pagesize, PROT_NONE);
    }

    pthread_mutex_lock(&allocation_lock);
    const unsigned long h = hash_ptr(base);
    unsigned long idx;
    for (int i = 0; i < MAX_TRACKED_ALLOCS; i++) {
        idx = (h + i) % MAX_TRACKED_ALLOCS;
        if (allocations[idx].addr == NULL) {
            allocations[idx] = (AllocationEntry){base, size, total};
            allocation_count++;
            break;
        }
    }
    pthread_mutex_unlock(&allocation_lock);

    send_json_event(EVENT_MALLOC, base, size);
    return base;
}

/**
 * @brief Replacement for free(), applies PROT_NONE to detect use-after-free.
 *
 * Adds the freed region to a tracking list. If mprotect fails, region is unmapped.
 */

void free(void* ptr) {
    if (!tracking_enabled || ptr == NULL || current_mode == MODE_PERF) {
        if (!real_free) real_free = dlsym(RTLD_NEXT, "free");
        real_free(ptr);
        return;
    }

    int found = 0;
    size_t requested = 0, alloc_size = 0;

    pthread_mutex_lock(&allocation_lock);
    const unsigned long h = hash_ptr(ptr);
    unsigned long idx;
    for (int i = 0; i < MAX_TRACKED_ALLOCS; i++)
    {
        idx = (h + i) % MAX_TRACKED_ALLOCS;
        if (allocations[idx].addr == ptr)
        {
            requested = allocations[idx].requested_size;
            alloc_size = allocations[idx].allocated_size;
            allocations[idx].addr = NULL;
            found = 1;
            allocation_count--;
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

/**
 * @brief Destructor function that runs on program exit.
 *
 * Performs cleanup: reports any detected memory leaks,
 * closes the analyzer socket, and resets internal state.
 */

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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>

// === Constants ===
#define GUARD_PATTERN 0xDEADBEEF
#define MAX_RECORDS 1024
#define GUARD_SIZE sizeof(int)
#define MAX_PROTECTED 1024

// === Memory Record Struct ===
typedef struct MemRecord {
    void* addr;
    size_t size;
    pthread_t tid;
    time_t timestamp;
    bool is_freed;
} MemRecord;

// === Globals ===
static MemRecord* mem_table[MAX_RECORDS];
static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;

static void* protected_ptrs[MAX_PROTECTED];
static size_t protected_sizes[MAX_PROTECTED];
static struct sigaction old_action;

void show_warning(const char* msg) { // Can be deleted once GUI works
    fprintf(stderr, "[WARNING] %s\n", msg); // headless warning
}

// === Guard Zone Logic ===
void set_guard(void* ptr, size_t size) {
    int* guard = (int*)((char*)ptr + size);
    *guard = GUARD_PATTERN;
}

bool check_guard(void* ptr, size_t size) {
    int* guard = (int*)((char*)ptr + size);
    return *guard == GUARD_PATTERN;
}

// === Allocation Tracker ===
void register_alloc(void* ptr, size_t size) {
    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (!mem_table[i]) {
            mem_table[i] = real_malloc(sizeof(MemRecord));
            mem_table[i]->addr = ptr;
            mem_table[i]->size = size;
            mem_table[i]->tid = pthread_self();
            mem_table[i]->timestamp = time(NULL);
            mem_table[i]->is_freed = false;
            break;
        }
    }
    pthread_mutex_unlock(&table_lock);
}

// === Dangling Pointer Protection ===
void add_protected(void* ptr, size_t size) {
    for (int i = 0; i < MAX_PROTECTED; i++) {
        if (!protected_ptrs[i]) {
            protected_ptrs[i] = ptr;
            protected_sizes[i] = size;
            //mprotect(ptr, size, PROT_NONE);

            int res = mprotect(ptr, size, PROT_NONE);
            fprintf(stderr, "Trying to protect %p (%lu bytes) - result: %d\n", ptr, size, res);
            break;
        }
    }
}

void segv_handler(int sig, siginfo_t* si, void* unused) {
    void* addr = si->si_addr;
    for (int i = 0; i < MAX_PROTECTED; i++) {
        if (protected_ptrs[i] && addr >= protected_ptrs[i] &&
            addr < (void*)((char*)protected_ptrs[i] + protected_sizes[i])) {
            show_warning("Dangling pointer access detected!");
            //return;   // Prevents crash whilst testing -> infinite loop
            break;    // Leads to crash whilst testing
        }
    }
    sigaction(SIGSEGV, &old_action, NULL);
    raise(SIGSEGV);
}

void init_segv_handler() {
    struct sigaction sa;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, &old_action);
}

// === Wrappers ===

void* malloc(size_t size) {
    if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
    void* ptr = real_malloc(size + GUARD_SIZE);
    if (!ptr) return NULL;
    set_guard(ptr, size);
    register_alloc(ptr, size);
    fprintf(stderr, "[WRAP] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    int (*real_posix_memalign)(void **, size_t, size_t);
    real_posix_memalign = dlsym(RTLD_NEXT, "posix_memalign");

    int result = real_posix_memalign(memptr, alignment, size);
    if(result == 0) {
        fprintf(stderr, "[WRAP] posix_memalign(%lu, %lu) = %p\n", alignment, size, *memptr);
        register_alloc(*memptr, size);
    }
    return result;
}

void free(void* ptr) {
    if (!ptr) return;
    if (!real_free) real_free = dlsym(RTLD_NEXT, "free");

    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (mem_table[i] && mem_table[i]->addr == ptr && !mem_table[i]->is_freed) {
            if (!check_guard(ptr, mem_table[i]->size)) {
                show_warning("Buffer overflow detected!");
            }
            mem_table[i]->is_freed = true;
            add_protected(ptr, mem_table[i]->size);
            break;
        }
    }
    pthread_mutex_unlock(&table_lock);

    fprintf(stderr, "[WRAP] free(%p)\n", ptr);
    real_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    void* ptr = malloc(nmemb * size);
    if (ptr) memset(ptr, 0, nmemb * size);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;

    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (mem_table[i] && mem_table[i]->addr == ptr) {
            size_t copy_size = size < mem_table[i]->size ? size : mem_table[i]->size;
            memcpy(new_ptr, ptr, copy_size);
            break;
        }
    }
    pthread_mutex_unlock(&table_lock);

    free(ptr);
    return new_ptr;
}

// === Leak & Fragmentation Checks ===
void check_leaks() {
    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (mem_table[i] && !mem_table[i]->is_freed) {
            fprintf(stderr, "[LEAK] %p (%lu bytes)\n", mem_table[i]->addr, mem_table[i]->size);
        }
    }
    pthread_mutex_unlock(&table_lock);
}

void check_fragmentation() {
    size_t used = 0, freed = 0, holes = 0;
    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_RECORDS; i++) {
        if (mem_table[i]) {
            if (mem_table[i]->is_freed) {
                freed += mem_table[i]->size;
                holes++;
            } else {
                used += mem_table[i]->size;
            }
        }
    }
    pthread_mutex_unlock(&table_lock);

    if (holes >= 10 && freed > used * 0.25) {
        show_warning("High memory fragmentation detected!");
    }
}

// === Startup and Cleanup ===
__attribute__((constructor)) void init_monitor() {
    init_segv_handler();
    fprintf(stderr, "[Memory Monitor] Initialized.\n");
}

__attribute__((destructor)) void shutdown_monitor() {
    check_leaks();
    check_fragmentation();
    fprintf(stderr, "[Memory Monitor] Shutdown complete.\n");
}

#include <memwrap.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

void enable_tracking() __attribute__((weak));

void test_simple_allocation() {
    printf("\n[TEST] Simple allocation and free\n");
    void* x = malloc(100);
    free(x);
}

void test_memory_leak() {
    printf("\n[TEST] Memory leak detection\n");
    void* p1 = malloc(64);
    if (!p1) {
        fprintf(stderr, "malloc failed\n");
        return;
    }

    void* p2 = malloc(128);
    if (!p2) {
        fprintf(stderr, "malloc failed\n");
        free(p1);
        return;
    }

    free(p1);
    // Intentionally NOT freeing p2
}

void test_buffer_overflow() {
    printf("\n[TEST] Buffer overflow detection\n");
    char* p3 = malloc(4096);
    strcpy(p3, "This is a test string.");
    p3[4096] = 'X';  // Write just past the allocated memory
    free(p3);
}

void test_dangling_pointer() {
    printf("\n[TEST] Dangling pointer detection\n");
    char* p = malloc(126);
    free(p);
    p[0] = 'X';       // accesses dangling pointer
}


void test_double_free() {
    printf("\n[TEST] Double free detection\n");
    void* p = malloc(35);
    if (!p) {
        fprintf(stderr, "malloc failed\n");
        return;
    }

    // First free:
    free(p);
    // Intentionally second free:
    free(p);
}

void test_fragmentation() {
    printf("\n[TEST] Fragmentation detection\n");

    const int num_blocks = 500;
    const size_t block_size = 4096;
    void* blocks[num_blocks];

    // Allocate many small blocks
    for (int i = 0; i < num_blocks; i++) {
        blocks[i] = malloc(block_size);
        if(!blocks[i]) {
            fprintf(stderr, "malloc failed at block %d\n", i);
            break;
        }
    }

    // Free every other block to simulate fragmentation
    for (int i = 0; i < num_blocks; i+=2) {
        free(blocks[i]);
    }

    printf("\n[TEST] Sleeping 12s to allow fragmentation detection...\n");
    sleep(12);

    // Free the rest
    for (int i = 1; i < num_blocks; i+=2) {
        free(blocks[i]);
    }

    printf("\n[TEST] Fragment detection complete\n");
}


int main() {
    printf("=== Starting test_alloc ===\n");

    if (enable_tracking) {
        enable_tracking();
    }


    // Test Memory Leak with Double Frees (and simple allocation):
    /*
    test_double_free();
    test_memory_leak();
    test_simple_allocation();
    */

    // Test Dangling Pointer:
    /*
    test_dangling_pointer();
    */


    // Test Buffer Overflow:
    /*
    test_buffer_overflow();
    */

    // Test Fragmentation:

    test_fragmentation();

    printf("\n[TEST] All tests completed.\n");
    return 0;
}

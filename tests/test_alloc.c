#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_simple_allocation() {
    printf("\n[TEST] Simple allocation and free\n");
    void* x = malloc(100);
    sleep(10);  // for testing multiple threads
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
    char* p4;
    posix_memalign((void**)&p4, 4096, 4096);
    free(p4);
    p4[0] = 'X';  // Access after free
}

int main() {
    printf("=== Starting test_alloc ===\n");

    // currently only this implemented:
    test_simple_allocation();

    /* after later added functionalities:
    test_memory_leak();
    test_buffer_overflow();
    test_dangling_pointer();
    */

    printf("\n[TEST] All tests completed.\n");
    return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("Starting test_alloc...\n");

    printf("Starting memory leak detection test...\n");
    void* p1 = malloc(64);
    if (!p1) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    void* p2 = malloc(128);
    if (!p2) {
        fprintf(stderr, "malloc failed\n");
        free(p1);
        return 1;
    }

    free(p1);

    printf("Starting buffer overflow detection test...\n");
    char* p3 = malloc(4096);
    strcpy(p3, "This is a test string.");
    p3[4096] = 'X';
    free(p3);

    printf("Starting dangling pointer detection test...\n");
    char* p4;
    posix_memalign((void**)&p4, 4096, 4096);
    free(p4);
    p4[0] = 'X';


    // Intentional memory leak: p2 is not freed
    // This will be caught by Valgrind

    printf("Test complete.\n");
    return 0;
}


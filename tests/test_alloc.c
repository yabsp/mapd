#include <stdlib.h>
#include <stdio.h>

int main() {
    printf("Starting test_alloc...\n");

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

    // Intentional memory leak: p2 is not freed
    // This will be caught by Valgrind

    printf("Test complete.\n");
    return 0;
}


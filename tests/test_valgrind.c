#include <stdlib.h>

int main() {
    void* leak = malloc(100); // Leak on purpose
    (void)leak;               // Avoid unused warning
    return 0;
}


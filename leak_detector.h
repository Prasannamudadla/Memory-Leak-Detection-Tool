#include "leak_detector.h"

int main() {
    char *ptr1 = malloc(1024); // Allocation 1
    char *ptr2 = malloc(2048); // Allocation 2

    free(ptr1);                // Free Allocation 1
    // Note: ptr2 is not freed, so it should appear as a leak

    return 0;                  // Memory report is generated at exit
}

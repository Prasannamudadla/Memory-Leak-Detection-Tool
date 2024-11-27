/*Mudadla Sai Prasanna : 2203120
  Maryada Harshini     : 2203118
  Venkatapuram Pooja   : 2203138
*/
#ifndef __LEAK_DETECTOR_H_
#define __LEAK_DETECTOR_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// Maximum number of memory allocations to track
#define LEAK_MEM_SIZE 1000

// Macro to print warnings with file and line information
#define _leak_warn(file, line, msg) \
    fprintf(stderr, "[WARNING] (%s:%d) %s\n", file, line, msg)

// Indicates if the leak detector has been initialized
static bool initialized = false;

// Struct to store information about each memory allocation
typedef struct {
    size_t address;  // Address of the allocated memory
    size_t size;     // Size of the allocation in bytes
    char file[255];  // File name where allocation occurred
    uint32_t line;   // Line number where allocation occurred
} Mem;

// Struct to store data about all tracked allocations
static struct MemData {
    Mem mem[LEAK_MEM_SIZE];  // Array to store memory allocation data
    uint32_t current;        // Current index in the array
    uint32_t allocations;    // Total number of allocations made
    uint32_t frees;          // Total number of frees made
    size_t total_allocated;  // Total bytes allocated
    size_t total_freed;      // Total bytes freed
} memoryData;

// Function to insert a memory allocation record
static bool _insert(void *ptr, size_t size, int line, char *file) {
    uint32_t i;
    const size_t address = (size_t)ptr;
    if ((i = memoryData.current) < LEAK_MEM_SIZE) {
        memoryData.mem[i].address = address;
        memoryData.mem[i].size = size;
        memoryData.mem[i].line = line;
        strncpy(memoryData.mem[i].file, file, sizeof(memoryData.mem[i].file) - 1);

        memoryData.current++;
        memoryData.allocations++;
        memoryData.total_allocated += size;
        return true;
    }
    return false; // Return false if the tracking array is full
}

// Function to remove a memory allocation record
static bool _delete(void *ptr) {
    const size_t address = (size_t)ptr;

    if (ptr != NULL) {
        for (uint32_t i = 0; i < memoryData.current; i++) {
            if (address == memoryData.mem[i].address) {
                memoryData.mem[i].address = 0; // Mark the record as freed
                memoryData.frees++;
                memoryData.total_freed += memoryData.mem[i].size;
                return true;
            }
        }
    }
    return false; // Return false if the record was not found
}

// Function to generate a report of memory usage and leaks
void _generate_report() {
    printf("\n========= MEMORY LEAK DETECTOR REPORT =========\n");
    printf("Total Allocations:      %d\n", memoryData.allocations);
    printf("Total Frees:            %d\n", memoryData.frees);
    printf("Total Memory Allocated: %lu bytes\n", memoryData.total_allocated);
    printf("Total Memory Freed:     %lu bytes\n", memoryData.total_freed);

    size_t leaked = memoryData.total_allocated - memoryData.total_freed;
    printf("Memory Leaked:          %lu bytes\n", leaked);

    if (memoryData.total_allocated > 0) {
        printf("Leak Percentage:        %.2f%%\n",
               (leaked * 100.0) / memoryData.total_allocated);
    }

    if (leaked > 0) {
        printf("\n======== LEAK DETAILS ========\n");
        for (uint32_t i = 0; i < memoryData.current; i++) {
            if (memoryData.mem[i].address != 0) { // Check if a memory block is still allocated
                printf("Leaked Memory:\n");
                printf("  - Address:  %p\n", (void *)memoryData.mem[i].address);
                printf("  - Size:     %lu bytes\n", memoryData.mem[i].size);
                printf("  - Location: %s:%d\n", memoryData.mem[i].file, memoryData.mem[i].line);
            }
        }
    } else {
        printf("\nNo memory leaks detected. All allocations were properly freed!\n");
    }
    printf("==============================================\n");
}

// Function to initialize the memory leak detector
void init() {
    if (!initialized) {
        atexit(_generate_report); // Register report generation function to run at program exit
        initialized = true;
    }
}

// Redefined `malloc` function with tracking
void *_malloc(size_t size, char *file, int line) {
    init();

    void *ptr = malloc(size);

    if (ptr == NULL) {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }

    _insert(ptr, size, line, file);
    return ptr;
}

// Redefined `calloc` function with tracking
void *_calloc(size_t num, size_t size, char *file, int line) {
    init();

    void *ptr = calloc(num, size);
    if (ptr == NULL) {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }

    _insert(ptr, num * size, line, file);
    return ptr;
}

// Redefined `realloc` function with tracking
void *_realloc(void *ptr, size_t size, char *file, int line) {
    void *new_ptr = realloc(ptr, size);

    if (new_ptr == NULL) {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }

    if (!_delete(ptr)) {
        _leak_warn(file, line, "Double free or invalid free detected");
        exit(EXIT_FAILURE);
    }
    _insert(new_ptr, size, line, file);

    return new_ptr;
}

// Redefined `free` function with tracking
void _free(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        _leak_warn(file, line, "Attempted to free a NULL pointer");
        return;
    }

    free(ptr);
    if (!_delete(ptr)) {
        _leak_warn(file, line, "Double free or invalid free detected");
        exit(EXIT_FAILURE);
    }
}

// Redefine standard memory functions to custom ones for tracking
#define malloc(size) _malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _calloc(num, size, __FILE__, __LINE__)
#define realloc(ptr, size) _realloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) _free(ptr, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __LEAK_DETECTOR_H_

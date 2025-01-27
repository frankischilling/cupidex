// vecstack.h

#ifndef VECSTACK_H
#define VECSTACK_H

#include <stddef.h> // For size_t

// Define the VecStack structure
typedef struct VecStack {
    void **items;      // Array of pointers to store stack items
    size_t size;       // Current number of items in the stack
    size_t capacity;   // Maximum capacity of the stack
} VecStack;

// Function to initialize the stack
void VecStack_init(VecStack *stack);

// Function to push an item onto the stack
void VecStack_push(VecStack *stack, void *item);

// Function to pop an item from the stack
void *VecStack_pop(VecStack *stack);

// Function to clean up the stack
void VecStack_bye(VecStack *stack);

#endif // VECSTACK_H

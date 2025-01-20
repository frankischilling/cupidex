// vecstack.c

#include "vecstack.h"
#include <stdlib.h> // For malloc, realloc, free
#include <stdio.h>  // For fprintf, stderr

#define INITIAL_CAPACITY 10

void VecStack_init(VecStack *stack) {
    if (stack == NULL) {
        fprintf(stderr, "VecStack_init: Provided stack pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    stack->size = 0;
    stack->capacity = INITIAL_CAPACITY;
    stack->items = malloc(sizeof(void*) * stack->capacity);
    if (stack->items == NULL) {
        fprintf(stderr, "VecStack_init: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
}

void VecStack_push(VecStack *stack, void *item) {
    if (stack == NULL) {
        fprintf(stderr, "VecStack_push: Provided stack pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    // Resize if necessary
    if (stack->size == stack->capacity) {
        stack->capacity *= 2;
        void **new_items = realloc(stack->items, sizeof(void*) * stack->capacity);
        if (new_items == NULL) {
            fprintf(stderr, "VecStack_push: Memory reallocation failed.\n");
            exit(EXIT_FAILURE);
        }
        stack->items = new_items;
    }

    stack->items[stack->size++] = item;
}

void *VecStack_pop(VecStack *stack) {
    if (stack == NULL) {
        fprintf(stderr, "VecStack_pop: Provided stack pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    if (stack->size == 0) {
        return NULL; // Stack is empty
    }

    return stack->items[--stack->size];
}

void VecStack_bye(VecStack *stack) {
    if (stack == NULL) {
        fprintf(stderr, "VecStack_bye: Provided stack pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

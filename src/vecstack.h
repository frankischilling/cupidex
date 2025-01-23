// ─────────────────────────────────────────────────────────────
// VecStack Header File
// Provides a dynamic stack implementation for generic pointers.
// ─────────────────────────────────────────────────────────────

#ifndef VECSTACK_H
#define VECSTACK_H

#include <stddef.h> // For size_t

// ─────────────────────────────────────────────────────────────
// Structure Definition
// ─────────────────────────────────────────────────────────────

/**
 * @struct VecStack
 * @brief A dynamic stack implementation for storing generic pointers.
 */
typedef struct VecStack {
    void **items;      ///< Array of pointers to store stack items.
    size_t size;       ///< Current number of items in the stack.
    size_t capacity;   ///< Maximum capacity of the stack before resizing.
} VecStack;

// ─────────────────────────────────────────────────────────────
// Function Declarations
// ─────────────────────────────────────────────────────────────

/**
 * @brief Initializes a VecStack instance.
 * @param stack Pointer to the VecStack structure to initialize.
 */
void VecStack_init(VecStack *stack);

/**
 * @brief Pushes an item onto the stack.
 * @param stack Pointer to the VecStack structure.
 * @param item Pointer to the item to push onto the stack.
 */
void VecStack_push(VecStack *stack, void *item);

/**
 * @brief Pops an item from the stack.
 * @param stack Pointer to the VecStack structure.
 * @return Pointer to the popped item, or NULL if the stack is empty.
 */
void *VecStack_pop(VecStack *stack);

/**
 * @brief Cleans up the stack by freeing allocated memory.
 * @param stack Pointer to the VecStack structure.
 */
void VecStack_bye(VecStack *stack);

#endif // VECSTACK_H

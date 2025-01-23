// ─────────────────────────────────────────────────────────────
// Vector Header File
// Provides a dynamic vector (resizable array) implementation.
// ─────────────────────────────────────────────────────────────

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h> // For size_t

// ─────────────────────────────────────────────────────────────
// Structure Definition
// ─────────────────────────────────────────────────────────────

/**
 * @struct VectorImpl
 * @brief Internal implementation details for Vector.
 * @note Do not access this structure directly.
 */
struct VectorImpl {
    size_t cap; ///< Capacity of the vector (maximum allocated elements).
    size_t len; ///< Current number of elements in the vector.
};

/**
 * @struct Vector
 * @brief A resizable array implementation for storing generic pointers.
 */
typedef struct {
    void **el; ///< Pointer to an array of elements.
    struct VectorImpl _PRIVATE_dont_access_PRIVATE_; ///< Internal metadata.
} Vector;

// ─────────────────────────────────────────────────────────────
// Function Declarations
// ─────────────────────────────────────────────────────────────

/**
 * @brief Creates a new vector with an initial capacity.
 * @param cap The initial capacity of the vector.
 * @return A new Vector instance.
 */
Vector Vector_new(size_t cap);

/**
 * @brief Frees memory allocated for the vector.
 * @param v Pointer to the vector to clean up.
 */
void Vector_bye(Vector *v);

/**
 * @brief Adds capacity to the vector without modifying its length.
 * @param v Pointer to the vector.
 * @param add Number of additional slots to allocate.
 */
void Vector_add(Vector *v, size_t add);

/**
 * @brief Sets the vector's length without freeing memory.
 * @param v Pointer to the vector.
 * @param len The new length of the vector.
 */
void Vector_set_len_no_free(Vector *v, size_t len);

/**
 * @brief Sets the vector's length and frees memory if necessary.
 * @param v Pointer to the vector.
 * @param len The new length of the vector.
 */
void Vector_set_len(Vector *v, size_t len);

/**
 * @brief Gets the current length of the vector.
 * @param v The vector instance.
 * @return The number of elements in the vector.
 */
size_t Vector_len(Vector v);

/**
 * @brief Ensures the vector's capacity is at least its length.
 * @param v Pointer to the vector.
 */
void Vector_min_cap(Vector *v);

/**
 * @brief Adjusts the vector's capacity to a reasonable size.
 * @param v Pointer to the vector.
 */
void Vector_sane_cap(Vector *v);

#endif // VECTOR_H

// src/core/common.h
#ifndef COMMON_H
#define COMMON_H

/**
 * @brief Macro to calculate the number of elements in a statically-sized array.
 *
 * This only works for arrays whose size is known at compile time. It does not work for dynamically
 * allocated arrays or pointers.
 *
 * @param a The array whose element count is to be calculated.
 * @return The number of elements in the array.
 */
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#endif
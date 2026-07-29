#ifndef MATH_H
#define MATH_H

/**
 * @brief Minimal vector types.
 *
 * Storage only for now, no operations. Named rather than using raw float
 * arrays so that call sites do not churn when actual math lands alongside
 * uniform buffers and transforms.
 */
typedef struct
{
    float x, y;
} Vec2;

typedef struct
{
    float x, y, z;
} Vec3;

#endif
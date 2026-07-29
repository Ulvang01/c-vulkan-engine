#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include "buffer.h"
#include "core/math.h"
#include <vulkan/vulkan.h>

typedef struct VulkanEngine VulkanEngine;

/**
 * @brief A mesh consisting of a vertex buffer and an index buffer.
 *
 * The vertex buffer contains the vertices of the mesh, and the index buffer
 * contains the indices that define the triangles of the mesh. The indexCount
 * field indicates how many indices are in the index buffer.
 */
typedef struct
{
    Buffer vertices;
    Buffer indices;
    uint32_t indexCount;
} Mesh;

/**
 * @brief One vertex as the shader expects it.
 *
 * Field order and types must match the layout(location = N) in
 * declarations in triangle.vert. Nothing checks this at compile time,
 * a mismatch shows up as garbage geometry rather than an error.
 */
typedef struct
{
    Vec2 pos;
    Vec3 color;
} Vertex;

/**
 * @brief Creates the vertex buffer and uploads the triangle's vertices.
 *
 * @param engine Requires physicalDevice and device to be set.
 * @return 0 on success, non-zero on failure.
 */
int createVertexBuffer(VulkanEngine *engine);

/**
 * @brief Creates the index buffer and uploads the triangle's indices.
 *
 * @param engine Requires physicalDevice and device to be set.
 * @return 0 on success, non-zero on failure.
 */
int createIndexBuffer(VulkanEngine *engine);

/**
 * @brief Describes the spacing and rate of vertex data for binding 0.
 */
VkVertexInputBindingDescription vertexGetBindingDescription(void);

/**
 * @brief Describes each attribute within a Vertex: which location, what
 *        format, and at what byte offset.
 *
 * @param out Array of at least 2 elements, filled in.
 * @return Number of descriptions written.
 */
uint32_t vertexGetAttributeDescriptions(VkVertexInputAttributeDescription *out);

#endif
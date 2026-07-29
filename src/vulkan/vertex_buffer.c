#include "vertex_buffer.h"
#include "buffer.h"
#include "core/common.h"
#include "core/log.h"
#include "vulkan_engine.h"
#include <stddef.h>
#include <string.h>

// The quad, now owned by the application instead of the shader.
// Winding is clockwise on screen to match VK_FRONT_FACE_CLOCKWISE in the
// rasterizer, reversing these silently culls the geometry.
static const Vertex vertices[] = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
                                   { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
                                   { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
                                   { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } } };

// Read three at a time to form a triangle. Vertices 0 and 2 are shared
// across both triangles, that sharing is the point of an index buffer.
static const uint16_t indices[] = {
    0, 1, 2, 2, 3, 0,
};

/**
 * @brief Uploads host data into a new device-local buffer via a staging buffer.
 *
 * Device-local memory is not CPU-writable, so the data goes into a
 * host-visible staging buffer first and is then copied across on the GPU.
 *
 * @param usage Usage flags for the destination, TRANSFER_DST_BIT is added
 *              automatically.
 * @param out   Receives the device-local buffer.
 * @return 0 on success, non-zero on failure.
 */
static int uploadToDeviceLocal(VulkanEngine *engine, const void *src, VkDeviceSize size,
                               VkBufferUsageFlags usage, Buffer *out)
{
    Buffer staging;
    if (bufferCreate(engine->device, engine->physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging) != 0)
    {
        return 1;
    }

    // Map, copy, unmap. HOST_COHERENT means no explicit flush is needed.
    void *data;
    vkMapMemory(engine->device, staging.memory, 0, size, 0, &data);
    memcpy(data, src, (size_t)size);
    vkUnmapMemory(engine->device, staging.memory);

    if (bufferCreate(engine->device, engine->physicalDevice, size,
                     usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     out) != 0)
    {
        bufferDestroy(engine->device, &staging);
        return 1;
    }

    int result =
        bufferCopy(engine->device, engine->commandPool, engine->graphicsQueue, &staging, out, size);

    bufferDestroy(engine->device, &staging);

    if (result != 0)
    {
        bufferDestroy(engine->device, out);
        return 1;
    }

    return 0;
}

int createVertexBuffer(VulkanEngine *engine)
{
    if (uploadToDeviceLocal(engine, vertices, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            &engine->mesh.vertices) != 0)
    {
        LOG_ERROR("Failed to create vertex buffer");
        return 1;
    }

    LOG_INFO("Vertex buffer created OK");
    return 0;
}

int createIndexBuffer(VulkanEngine *engine)
{
    if (uploadToDeviceLocal(engine, indices, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            &engine->mesh.indices) != 0)
    {
        LOG_ERROR("Failed to create index buffer");
        return 1;
    }

    engine->mesh.indexCount = ARRAY_COUNT(indices);

    LOG_INFO("Index buffer created OK");
    return 0;
}

VkVertexInputBindingDescription vertexGetBindingDescription(void)
{
    VkVertexInputBindingDescription binding = { 0 };
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    // VERTEX means advance one Vertex per vertex. INSTANCE would advance
    // per instance instead, used for instanced rendering.
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

uint32_t vertexGetAttributeDescriptions(VkVertexInputAttributeDescription *out)
{
    // location 0: vec2 inPosition
    out[0].binding = 0;
    out[0].location = 0;
    out[0].format = VK_FORMAT_R32G32_SFLOAT;
    out[0].offset = offsetof(Vertex, pos);

    // location 1: vec3 inColor
    out[1].binding = 0;
    out[1].location = 1;
    out[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    out[1].offset = offsetof(Vertex, color);

    return 2;
}
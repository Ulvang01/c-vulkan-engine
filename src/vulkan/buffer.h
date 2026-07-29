#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan.h>

typedef struct
{
    VkBuffer buffer;
    VkDeviceMemory memory;
} Buffer;

int bufferCreate(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer *out);

int bufferCopy(VkDevice device, VkCommandPool pool, VkQueue queue, Buffer *src, Buffer *dst,
               VkDeviceSize size);

void bufferDestroy(VkDevice device, Buffer *buffer);

#endif
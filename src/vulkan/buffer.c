#include "buffer.h"
#include "core/log.h"
#include <stdbool.h>

/**
 * @brief Finds a memory type on the physical device satisfying both a
 *        type filter and a set of required properties.
 *
 * The device exposes several memory types with different characteristics
 * (device-local, host-visible, cached). vkGetBufferMemoryRequirements
 * returns a bitmask of which types are valid for a given buffer, and we
 * pick the first one that also has the properties we need.
 *
 * @return Index of a suitable memory type, or UINT32_MAX if none found.
 */
static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                               VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        bool typeAllowed = (typeFilter & (1 << i)) != 0;
        bool hasProperties =
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties;

        if (typeAllowed && hasProperties)
            return i;
    }

    return UINT32_MAX;
}

int bufferCreate(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                 VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer *out)
{
    VkBufferCreateInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, NULL, &out->buffer) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create buffer");
        return 1;
    }

    // vkCreateBuffer allocates nothing, it only describes a region. Ask what
    // backing memory it needs, which may exceed the requested size due to
    // alignment.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, out->buffer, &memRequirements);

    uint32_t memoryType =
        findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (memoryType == UINT32_MAX)
    {
        LOG_ERROR("No suitable memory type for buffer");
        vkDestroyBuffer(device, out->buffer, NULL);
        return 1;
    }

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryType;

    if (vkAllocateMemory(device, &allocInfo, NULL, &out->memory) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to allocate buffer memory");
        vkDestroyBuffer(device, out->buffer, NULL);
        return 1;
    }

    vkBindBufferMemory(device, out->buffer, out->memory, 0);
    return 0;
}

int bufferCopy(VkDevice device, VkCommandPool pool, VkQueue queue, Buffer *src, Buffer *dst,
               VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to allocate command buffer for buffer copy");
        return 1;
    }

    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Tells the driver this buffer is submitted once and then discarded,
    // which lets it skip some bookkeeping
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = { 0 };
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src->buffer, dst->buffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    // Blunt but fine for a one-off startup upload: stalls the whole queue
    // until the copy completes. Streaming assets at runtime would use a
    // fence, or a dedicated transfer queue.
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);

    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to submit buffer copy: %d", result);
        return 1;
    }

    return 0;
}

void bufferDestroy(VkDevice device, Buffer *buffer)
{
    // Buffer first, then its memory. Freeing memory still bound to a live
    // buffer is the wrong order.
    vkDestroyBuffer(device, buffer->buffer, NULL);
    vkFreeMemory(device, buffer->memory, NULL);
}
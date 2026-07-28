#ifndef COMMANDS_H
#define COMMANDS_H

#include <vulkan/vulkan.h>

typedef struct VulkanEngine VulkanEngine;

/**
 * @brief Creates the command pool that command buffers are allocated from.
 *
 * A pool is tied to one queue family, buffers allocated from it can only be
 * submitted to queues from that family. Ours is the graphics family.
 *
 * @param engine Requires physicalDevice, device, and surface to be set.
 * @return 0 on success, non-zero on failure.
 */
int createCommandPool(VulkanEngine *engine);

/**
 * @brief Allocates the command buffer used to record draw commands.
 *
 * @param engine Requires engine->commandPool to exist.
 * @return 0 on success, non-zero on failure.
 */
int createCommandBuffer(VulkanEngine *engine);

/**
 * @brief Records the commands to draw a single frame into the command buffer.
 *
 * This is called once per frame, after acquiring a swapchain image and before
 * submitting the command buffer to the graphics queue.
 *
 * @param engine        Requires engine->commandBuffer, engine->renderPass,
 *                      and engine->swapchain.framebuffers to exist.
 * @param commandBuffer The command buffer to record into. Usually engine->commandBuffer.
 * @param imageIndex    The index of the swapchain image to render into.
 * @return 0 on success, non-zero on failure.
 */
int recordCommandBuffer(VulkanEngine *engine, VkCommandBuffer commandBuffer, uint32_t imageIndex);

#endif
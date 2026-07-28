#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h>

typedef struct VulkanEngine VulkanEngine;

/**
 * @brief Creates the semaphores and fence used to synchronize a frame.
 *
 * @param engine Requires engine->device to be set.
 * @return 0 on success, non-zero on failure.
 */
int createSyncObjects(VulkanEngine *engine);

/**
 * @brief Renders and presents a single frame.
 *
 * Called once per iteration of the main loop.
 *
 * @param engine A fully initialized engine.
 * @return 0 on success, non-zero on failure.
 */
int drawFrame(VulkanEngine *engine);

#endif
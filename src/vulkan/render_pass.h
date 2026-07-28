#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>

typedef struct VulkanEngine VulkanEngine;

/**
 * @brief Creates the render pass describing how the swapchain image is
 *        treated across a frame: cleared on load, stored on finish, and
 *        transitioned into a presentable layout at the end.
 *
 * @param engine Requires engine->device and engine->swapchain.imageFormat.
 * @return 0 on success, non-zero on failure.
 */
int createRenderPass(VulkanEngine *engine);

#endif
#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "swapchain.h"
#include <GLFW/glfw3.h>

// Two lets the CPU record frame N+1 while the GPU works on frame N.
// Higher values add latency for little gain.
#define MAX_FRAMES_IN_FLIGHT 2

/**
 * @brief Everything belonging to one in-flight frame.
 *
 * Grouped because all three are always accessed together with the same
 * index, parallel arrays would let them drift.
 */
typedef struct
{
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkFence inFlightFence;
} FrameData;

/*
 * @brief Structure representing the Vulkan engine, containing the Vulkan instance, selected
 * physical device, and other Vulkan objects.
 */
typedef struct VulkanEngine
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    SwapchainState swapchainState;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    FrameData frames[MAX_FRAMES_IN_FLIGHT];
    uint32_t currentFrame;
    bool framebufferResized;
} VulkanEngine;

/**
 * @brief Runs the full Vulkan setup sequence: instance, then physical device.
 *
 * Each step depends on the previous one succeeding, so this stops and
 * returns early on the first failure rather than continuing with a
 * partially-initialized engine.
 *
 * @param engine Pointer to the VulkanEngine structure to initialize.
 * @return 0 on success, non-zero on failure.
 */
int vulkanEngineInit(VulkanEngine *engine, GLFWwindow *window);

/**
 * @brief Destroys every Vulkan object created by vulkanEngineInit, in
 *        reverse creation order.
 *
 * @warning Any new object added to vulkanEngineInit needs a matching
 *          destroy here, placed to preserve reverse ordering. Leaks and
 *          out-of-order destruction both surface as validation errors
 *          at teardown.
 *
 * @param engine Pointer to the VulkanEngine structure to clean up.
 */
void vulkanEngineCleanup(VulkanEngine *engine);

#endif

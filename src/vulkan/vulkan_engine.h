#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include "swapchain.h"
#include <GLFW/glfw3.h>

/*
 * @brief Structure representing the Vulkan engine, containing the Vulkan instance and selected
 * physical device.
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
    SwapchainState swapchain;
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
 * @brief Cleans up the Vulkan engine by destroying the Vulkan instance.
 *
 * @param engine Pointer to the VulkanEngine structure to clean up.
 */
void vulkanEngineCleanup(VulkanEngine *engine);

#endif

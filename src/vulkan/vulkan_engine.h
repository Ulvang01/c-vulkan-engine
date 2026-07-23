#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/*
 * @brief Structure representing the Vulkan engine, containing the Vulkan instance and selected
 * physical device.
 */
typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
} VulkanEngine;

/**
 * @brief Initializes the Vulkan engine by creating a Vulkan instance and selecting a physical
 * device.
 *
 * @param engine Pointer to the VulkanEngine structure to initialize.
 * @return 0 on success, non-zero on failure.
 */
int vulkanEngineInit(VulkanEngine *engine);

/**
 * @brief Cleans up the Vulkan engine by destroying the Vulkan instance.
 *
 * @param engine Pointer to the VulkanEngine structure to clean up.
 */
void vulkanEngineCleanup(VulkanEngine *engine);

#endif

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
} VulkanEngine;

int vulkanEngineInit(VulkanEngine *engine);
void vulkanEngineCleanup(VulkanEngine *engine);

#endif

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

// Forward declaration, we only need a pointer to this type here,
// not its full definition, avoids circular include with vulkan_engine.h
typedef struct VulkanEngine VulkanEngine;

typedef struct
{
    VkSwapchainKHR swapchain;
    VkImage *images;
    uint32_t imageCount;
    VkFormat imageFormat;
    VkExtent2D extent;
} SwapchainState;

typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formatCount;
    VkPresentModeKHR *presentModes;
    uint32_t presentModeCount;
} SwapchainSupportDetails;

int querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface,
                          SwapchainSupportDetails *details);

bool isSwapchainAdequate(SwapchainSupportDetails *details);

int createSwapchain(VulkanEngine *engine, GLFWwindow *window);

void freeSwapchainSupportDetails(SwapchainSupportDetails *details);

#endif
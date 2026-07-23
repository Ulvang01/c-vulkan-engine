#include "queue_families.h"
#include <stdlib.h>

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = { 0 };

    // Query the number of queue families available on the physical device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    // Allocate memory for the queue family properties
    // Get the queue family properties
    VkQueueFamilyProperties *queueFamilies =
        malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily.value = i;
            indices.graphicsFamily.hasValue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily.value = i;
            indices.presentFamily.hasValue = true;
        }

        // Note: no early break here, unlike the original graphics-only
        // version, since graphics and present support might land on
        // different queue families, we need to check all of them
    }

    free(queueFamilies);
    return indices;
}

bool queueFamilyIndicesIsComplete(QueueFamilyIndices indices)
{
    return indices.graphicsFamily.hasValue && indices.presentFamily.hasValue;
}

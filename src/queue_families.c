#include "queue_families.h"
#include <stdlib.h>

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = {0};

    // Query the number of queue families available on the physical device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    // Allocate memory for the queue family properties
    // Get the queue family properties
    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily.value = i;
            indices.graphicsFamily.hasValue = true;
            break;
        }
    }

    free(queueFamilies);
    return indices;
}

bool queueFamilyIndicesIsComplete(QueueFamilyIndices indices)
{
    return indices.graphicsFamily.hasValue;
    // later: && indices.presentFamily.hasValue
}

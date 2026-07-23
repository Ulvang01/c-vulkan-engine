#ifndef QUEUE_FAMILIES_H
#define QUEUE_FAMILIES_H

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

/**
 * @brief Represents an optional uint32_t value, indicating whether a value is present.
 */
typedef struct
{
    uint32_t value;
    bool hasValue;
} OptionalUint32;

/**
 * @brief Represents the indices of queue families for a Vulkan physical device.
 */
typedef struct
{
    OptionalUint32 graphicsFamily;
    OptionalUint32 presentFamily;
} QueueFamilyIndices;

/**
 * @brief Queries a physical device for queue families and finds
 *        one supporting graphics operations.
 * @param device The physical device to query.
 * @return QueueFamilyIndices with graphicsFamily set if found.
 */
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief Checks if the QueueFamilyIndices structure has all required queue families.
 * @param indices The QueueFamilyIndices to check.
 * @return true if all required queue families are present, false otherwise.
 */
bool queueFamilyIndicesIsComplete(QueueFamilyIndices indices);

#endif

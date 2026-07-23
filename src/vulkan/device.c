#include "device.h"
#include "core/log.h"
#include "queue_families.h"
#include "swapchain.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Checks if a physical device supports the required extensions.
 *
 * Currently, the only required extension is VK_KHR_swapchain, which is
 * necessary for presenting images to a window surface.
 *
 * @param device The Vulkan physical device to check.
 * @return true if all required extensions are supported, false otherwise.
 */
static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions =
        malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    bool found = false;
    for (uint32_t i = 0; i < extensionCount; i++)
    {
        if (strcmp(availableExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            found = true;
            break;
        }
    }

    free(availableExtensions);
    return found;
}

/**
 * @brief Scores a physical device's suitability for this application.
 *
 * Devices missing a graphics queue family are hard-disqualified (score 0),
 * since that's a strict requirement, not a preference. Among devices that
 * pass, discrete GPUs are strongly preferred over integrated ones, with
 * VRAM size as a minor tiebreaker.
 *
 * @param device The Vulkan physical device to rate.
 * @return An integer score; 0 means disqualified, higher is better.
 */
static int rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    // Hard requirement: no graphics/present queue means this device
    // is unusable, full stop, regardless of other merits
    if (!queueFamilyIndicesIsComplete(indices))
    {
        return 0;
    }

    // Hard requirement: without VK_KHR_swapchain support, this device
    // cannot present images to a window at all, disqualify immediately
    if (!checkDeviceExtensionSupport(device))
    {
        return 0;
    }

    // Hard requirement: the swapchain must support at least one format
    // and one present mode. The extension can technically be present
    // while still reporting zero usable formats/present modes for this
    // specific surface (allowed by spec on unusual hardware), in which
    // case a swapchain still can't actually be created, so this is a
    // real disqualifier, not just a scoring nudge.
    SwapchainSupportDetails swapchainDetails = { 0 };
    querySwapchainSupport(device, surface, &swapchainDetails);
    bool swapchainAdequate = isSwapchainAdequate(&swapchainDetails);
    freeSwapchainSupportDetails(&swapchainDetails);

    if (!swapchainAdequate)
    {
        return 0;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);

    int score = 0;

    // Discrete GPUs meaningfully outperform integrated ones for rendering
    // workloads, this bonus is large enough to dominate the score outright
    // rather than just nudge it, an integrated GPU with lots of shared
    // system RAM should never outscore a discrete GPU
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 100000;
    }
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 1000;
    }

    // Sum device-local heaps as a minor tiebreaker only, used to choose
    // between two GPUs of the same type (e.g. two discrete GPUs). Divided
    // down to keep its weight small relative to the type bonus above.
    // Note: integrated GPUs often report shared system RAM as "device-local"
    // too, so this isn't a reliable signal for integrated GPUs specifically,
    // it's only meaningful as a tiebreaker among devices of the same type.
    uint64_t totalMemMB = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            totalMemMB += memProps.memoryHeaps[i].size / (1024 * 1024);
        }
    }
    score += (int)(totalMemMB / 100);

    LOG_DEBUG("  [candidate] %s -> score %d", props.deviceName, score);

    return score;
}

int pickPhysicalDevice(VulkanEngine *engine)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, NULL);

    if (deviceCount == 0)
    {
        LOG_ERROR("No GPUs with Vulkan support found");
        return 1;
    }

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, devices);

    engine->physicalDevice = VK_NULL_HANDLE;
    int bestScore = -1;

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        int score = rateDeviceSuitability(devices[i], engine->surface);
        if (score > bestScore)
        {
            bestScore = score;
            engine->physicalDevice = devices[i];
        }
    }

    free(devices);

    if (engine->physicalDevice == VK_NULL_HANDLE || bestScore == 0)
    {
        LOG_ERROR("Failed to find a suitable GPU");
        return 1;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(engine->physicalDevice, &props);
    LOG_INFO("Using GPU: %s (score: %d)", props.deviceName, bestScore);

    return 0;
}

int createLogicalDevice(VulkanEngine *engine)
{
    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);

    float queuePriority = 1.0f;

    // Build a list of unique queue families we need queues from.
    // graphicsFamily and presentFamily are often the same index, but
    // aren't guaranteed to be, so we only create one VkDeviceQueueCreateInfo
    // per distinct family, not one per requirement.
    uint32_t uniqueFamilies[2];
    uint32_t uniqueFamilyCount = 0;

    uniqueFamilies[uniqueFamilyCount++] = indices.graphicsFamily.value;
    if (indices.presentFamily.value != indices.graphicsFamily.value)
    {
        uniqueFamilies[uniqueFamilyCount++] = indices.presentFamily.value;
    }

    VkDeviceQueueCreateInfo queueCreateInfos[2] = { 0 };
    for (uint32_t i = 0; i < uniqueFamilyCount; i++)
    {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueFamilies[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = { 0 };

    VkDeviceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = uniqueFamilyCount;
    createInfo.pEnabledFeatures = &deviceFeatures;

    static const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(engine->physicalDevice, &createInfo, NULL, &engine->device) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create logical device");
        return 1;
    }
    LOG_INFO("Logical device created OK");

    vkGetDeviceQueue(engine->device, indices.graphicsFamily.value, 0, &engine->graphicsQueue);
    vkGetDeviceQueue(engine->device, indices.presentFamily.value, 0, &engine->presentQueue);

    return 0;
}
#include "vulkan_engine.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Creates a Vulkan instance and stores it in the VulkanEngine structure.
 * @param engine Pointer to the VulkanEngine structure to store the created instance.
 * @return 0 on success, non-zero on failure.
 */
static int createInstance(VulkanEngine *engine)
{
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkResult result = vkCreateInstance(&createInfo, NULL, &engine->instance);
    if (result != VK_SUCCESS)
    {
        printf("vkCreateInstance failed: %d\n", result);
        return 1;
    }
    printf("Vulkan instance created OK\n");
    return 0;
}

/**
 * @brief Rates the suitability of a Vulkan physical device based on its properties and memory.
 * @param device The Vulkan physical device to rate.
 * @return An integer score representing the device's suitability.
 */
static int rateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);

    int score = 0;

    // Discrete GPUs still meaningfully outperform integrated ones,
    // this should dominate the score, not just nudge it
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 100000;
    }
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 1000;
    }

    // Sum device-local heaps as a minor tiebreaker only.
    // Note: integrated GPUs often report system RAM here too,
    // so this isn't reliable as a primary signal, keep its
    // weight small relative to the discrete/integrated bonus above.
    uint64_t totalMemMB = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            totalMemMB += memProps.memoryHeaps[i].size / (1024 * 1024);
        }
    }
    score += (int)(totalMemMB / 100); // scaled down, minor factor only

    printf("  [candidate] %s -> score %d\n", props.deviceName, score);

    return score;
}

/**
 * @brief Picks the most suitable Vulkan physical device and stores it in the VulkanEngine structure.
 * @param engine Pointer to the VulkanEngine structure to store the selected physical device.
 * @return 0 on success, non-zero on failure.
 */
static int pickPhysicalDevice(VulkanEngine *engine)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, NULL);

    if (deviceCount == 0)
    {
        printf("No GPUs with Vulkan support found\n");
        return 1;
    }

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, devices);

    engine->physicalDevice = VK_NULL_HANDLE;
    int bestScore = -1;

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        int score = rateDeviceSuitability(devices[i]);
        if (score > bestScore)
        {
            bestScore = score;
            engine->physicalDevice = devices[i];
        }
    }

    free(devices);

    if (engine->physicalDevice == VK_NULL_HANDLE)
    {
        printf("Failed to find a suitable GPU\n");
        return 1;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(engine->physicalDevice, &props);
    printf("Using GPU: %s (score: %d)\n", props.deviceName, bestScore);

    return 0;
}

int vulkanEngineInit(VulkanEngine *engine)
{
    if (createInstance(engine) != 0)
        return 1;
    if (pickPhysicalDevice(engine) != 0)
        return 1;
    return 0;
}

void vulkanEngineCleanup(VulkanEngine *engine)
{
    vkDestroyInstance(engine->instance, NULL);
}

#include "vulkan_engine.h"
#include <stdio.h>
#include <stdlib.h>

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

    printf("Found %u GPU(s)\n", deviceCount);

    // pick one, simplest for now: just take the first one
    engine->physicalDevice = devices[0];

    VkPhysicalDeviceProperties selectedDeviceProps;
    vkGetPhysicalDeviceProperties(engine->physicalDevice, &selectedDeviceProps);
    printf("Using GPU: %s\n", selectedDeviceProps.deviceName);

    free(devices);
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
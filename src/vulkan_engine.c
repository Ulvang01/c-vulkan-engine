#include "vulkan_engine.h"
#include "log.h"
#include "queue_families.h"
#include <stdio.h>
#include <stdlib.h>

static const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

/**
 * @brief Creates a Vulkan instance and stores it in the VulkanEngine structure.
 *
 * The instance is Vulkan's entry point, everything else (physical devices,
 * logical devices, surfaces) is created through or in relation to it.
 * This also enables the validation layer and the debug utils extension,
 * required for the debug messenger set up separately.
 *
 * @param engine Pointer to the VulkanEngine structure to store the created instance.
 * @return 0 on success, non-zero on failure.
 */
static int createInstance(VulkanEngine *engine)
{
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "c-vulkan-engine";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const char *extensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

    VkInstanceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;

    VkResult result = vkCreateInstance(&createInfo, NULL, &engine->instance);
    if (result != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateInstance failed: %d", result);
        return 1;
    }
    LOG_INFO("Vulkan instance created OK");
    return 0;
}

/**
 * @brief Receives validation layer messages from Vulkan and routes them
 *        through our own logger instead of Vulkan's default output.
 *
 * Vulkan calls this function itself whenever the validation layer catches
 * something worth reporting. We don't call it directly, we just hand its
 * address to Vulkan (this happens once the debug messenger is created).
 *
 * @param severity     How serious the message is (error, warning, info, verbose).
 * @param type         What kind of message this is (general, validation, performance).
 * @param callbackData Contains the actual message text (callbackData->pMessage).
 * @param userData     Unused here, would carry custom context if we passed any in.
 * @return VK_FALSE always, returning VK_TRUE would abort the Vulkan call
 *         that triggered this message, which we don't want.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
              const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[validation] %s", callbackData->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARN("[validation] %s", callbackData->pMessage);
    }
    else
    {
        LOG_DEBUG("[validation] %s", callbackData->pMessage);
    }
    return VK_FALSE;
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
static int rateDeviceSuitability(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    // Hard requirement, not a scoring factor: no graphics queue means
    // this device is unusable, full stop, regardless of other merits
    if (!queueFamilyIndicesIsComplete(indices))
    {
        return 0;
    }

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

    LOG_DEBUG("  [candidate] %s -> score %d", props.deviceName, score);

    return score;
}

/**
 * @brief Enumerates all Vulkan-capable physical devices and picks the
 *        best-scoring one (see rateDeviceSuitability), storing it in
 *        the VulkanEngine structure.
 *
 * @param engine Pointer to the VulkanEngine structure to store the selected physical device.
 * @return 0 on success, non-zero on failure (no devices, or none suitable).
 */
static int pickPhysicalDevice(VulkanEngine *engine)
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
        int score = rateDeviceSuitability(devices[i]);
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

/**
 * @brief Runs the full Vulkan setup sequence: instance, then physical device.
 *
 * Each step depends on the previous one succeeding, so this stops and
 * returns early on the first failure rather than continuing with a
 * partially-initialized engine.
 *
 * @note setupDebugMessenger and createLogicalDevice are not yet implemented,
 *       hence commented out below. Debug messenger will use debugCallback
 *       above once added.
 *
 * @param engine Pointer to the VulkanEngine structure to initialize.
 * @return 0 on success, non-zero on failure.
 */
int vulkanEngineInit(VulkanEngine *engine)
{
    if (createInstance(engine) != 0)
        return 1;
    // if (setupDebugMessenger(engine) != 0)
    //     return 1;
    if (pickPhysicalDevice(engine) != 0)
        return 1;
    // if (createLogicalDevice(engine) != 0)
    //     return 1;
    return 0;
}

/**
 * @brief Tears down everything created by vulkanEngineInit, in reverse order.
 * @param engine Pointer to the VulkanEngine structure to clean up.
 */
void vulkanEngineCleanup(VulkanEngine *engine) { vkDestroyInstance(engine->instance, NULL); }

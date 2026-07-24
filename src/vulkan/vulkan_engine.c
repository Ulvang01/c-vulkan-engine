#include "vulkan_engine.h"
#include "core/log.h"
#include "debug.h"
#include "device.h"
#include "surface.h"
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

    // Ask GLFW which instance extensions it needs for surface creation
    // on this platform (handles Windows/Linux/Mac differences for us)
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Combine GLFW's required extensions with our own debug extension
    uint32_t extensionCount = glfwExtensionCount + 1;
    const char **extensions = malloc(sizeof(const char *) * extensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        extensions[i] = glfwExtensions[i];
    }
    extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkInstanceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;

    VkResult result = vkCreateInstance(&createInfo, NULL, &engine->instance);
    free(extensions);

    if (result != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateInstance failed: %d", result);
        return 1;
    }
    LOG_INFO("Vulkan instance created OK");
    return 0;
}

int vulkanEngineInit(VulkanEngine *engine, GLFWwindow *window)
{
    if (createInstance(engine) != 0)
        return 1;
    if (setupDebugMessenger(engine) != 0)
        return 1;
    if (createSurface(engine, window) != 0)
        return 1;
    if (pickPhysicalDevice(engine) != 0)
        return 1;
    if (createLogicalDevice(engine) != 0)
        return 1;
    if (createSwapchain(engine, window) != 0)
        return 1;
    if (createImageViews(engine) != 0)
        return 1;
    return 0;
}

void vulkanEngineCleanup(VulkanEngine *engine)
{
    for (uint32_t i = 0; i < engine->swapchain.imageCount; i++)
    {
        vkDestroyImageView(engine->device, engine->swapchain.imageViews[i], NULL);
    }
    free(engine->swapchain.imageViews);
    free(engine->swapchain.images);
    vkDestroySwapchainKHR(engine->device, engine->swapchain.swapchain, NULL);
    vkDestroyDevice(engine->device, NULL);
    destroyDebugMessenger(engine);
    vkDestroySurfaceKHR(engine->instance, engine->surface, NULL);
    vkDestroyInstance(engine->instance, NULL);
}

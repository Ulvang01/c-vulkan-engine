#include "debug.h"
#include "core/log.h"

/**
 * @brief Receives validation layer messages from Vulkan and routes them
 *        through our own logger instead of Vulkan's default output.
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

int setupDebugMessenger(VulkanEngine *engine)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(engine->instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");

    if (!createDebugMessenger)
    {
        LOG_ERROR("vkCreateDebugUtilsMessengerEXT not available");
        return 1;
    }

    if (createDebugMessenger(engine->instance, &createInfo, NULL, &engine->debugMessenger) !=
        VK_SUCCESS)
    {
        LOG_ERROR("Failed to set up debug messenger");
        return 1;
    }

    LOG_INFO("Debug messenger set up OK");
    return 0;
}

void destroyDebugMessenger(VulkanEngine *engine)
{
    PFN_vkDestroyDebugUtilsMessengerEXT destroyFn =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            engine->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyFn)
    {
        destroyFn(engine->instance, engine->debugMessenger, NULL);
    }
}

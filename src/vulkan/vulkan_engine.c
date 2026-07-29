#include "vulkan_engine.h"
#include "commands.h"
#include "core/log.h"
#include "debug.h"
#include "device.h"
#include "pipeline.h"
#include "render_pass.h"
#include "renderer.h"
#include "surface.h"
#include "swapchain.h"
#include <stdlib.h>

static const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

/**
 * @brief GLFW callback fired when the window's framebuffer is resized.
 *
 * Sets a flag rather than recreating anything directly, GLFW may fire this
 * mid-frame and recreation has to happen at a safe point in drawFrame.
 *
 * Some drivers never return VK_ERROR_OUT_OF_DATE_KHR on resize, which is
 * why this exists alongside the return-code checks rather than instead of
 * them.
 */
static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    VulkanEngine *engine = glfwGetWindowUserPointer(window);
    if (!engine)
        return;

    engine->framebufferResized = true;

    // Windows blocks inside glfwPollEvents for the whole duration of an
    // edge drag, so the main loop cannot draw. This callback does still
    // fire during that modal loop, so rendering from here is what keeps
    // the window live while it is being dragged.
    if (recreateSwapchain(engine, window) == 0)
    {
        engine->framebufferResized = false;
        drawFrame(engine, window);
    }
}

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
    if (createRenderPass(engine) != 0)
        return 1;
    if (createGraphicsPipeline(engine) != 0)
        return 1;
    if (createFramebuffers(engine) != 0)
        return 1;
    if (createRenderFinishedSemaphores(engine) != 0)
        return 1;
    if (createCommandPool(engine) != 0)
        return 1;
    if (createCommandBuffer(engine) != 0)
        return 1;
    if (createSyncObjects(engine) != 0)
        return 1;

    // The callback needs to reach the engine, GLFW's only mechanism for
    // that is the window user pointer
    glfwSetWindowUserPointer(window, engine);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    return 0;
}

void vulkanEngineCleanup(VulkanEngine *engine)
{
    // Destroyed in reverse creation order. Vulkan requires a child object to
    // be destroyed before its parent, so this mirrors vulkanEngineInit, read
    // bottom-up to compare the two.

    // Everything sized by or derived from the swapchain: framebuffers, image
    // views, per-image semaphores, and the swapchain itself. Shared with the
    // resize path.
    cleanupSwapchain(engine);

    vkDestroySemaphore(engine->device, engine->imageAvailableSemaphore, NULL);
    vkDestroyFence(engine->device, engine->inFlightFence, NULL);

    // Command buffers are owned by the pool and freed with it
    vkDestroyCommandPool(engine->device, engine->commandPool, NULL);

    vkDestroyPipeline(engine->device, engine->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(engine->device, engine->pipelineLayout, NULL);
    vkDestroyRenderPass(engine->device, engine->renderPass, NULL);

    vkDestroyDevice(engine->device, NULL);

    // Instance-level objects, order between these two does not matter,
    // only that both come before the instance itself
    destroyDebugMessenger(engine);
    vkDestroySurfaceKHR(engine->instance, engine->surface, NULL);

    vkDestroyInstance(engine->instance, NULL);

    LOG_INFO("Vulkan engine cleaned up OK");
}

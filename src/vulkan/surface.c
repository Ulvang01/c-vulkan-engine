#include "surface.h"
#include "core/log.h"

int createSurface(VulkanEngine *engine, GLFWwindow *window)
{
    VkResult result = glfwCreateWindowSurface(engine->instance, window, NULL, &engine->surface);
    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create window surface: %d", result);
        return 1;
    }
    LOG_INFO("Window surface created OK");
    return 0;
}
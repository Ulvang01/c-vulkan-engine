#include "core/log.h"
#include "core/window.h"
#include "vulkan/renderer.h"
#include "vulkan/vulkan_engine.h"
#include <stdlib.h>
#include <string.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

int main(void)
{
    const char *logLevel = getenv("LOG_LEVEL");
    if (logLevel && strcmp(logLevel, "debug") == 0)
    {
        logSetMinLevel(LOG_DEBUG);
    }

    GLFWwindow *window = windowCreate(WINDOW_WIDTH, WINDOW_HEIGHT, "c_vulkan_engine");
    if (!window)
        return 1;

    VulkanEngine engine = { 0 };
    if (vulkanEngineInit(&engine, window) != 0)
    {
        windowDestroy(window);
        return 1;
    }

    // Main loop: runs until the user closes the window.
    // Nothing is drawn yet, this just keeps the window alive and responsive.
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame(&engine);
    }

    // The loop can exit while the GPU is still working. Destroying objects
    // out from under in-flight work is a validation error.
    vkDeviceWaitIdle(engine.device);

    vulkanEngineCleanup(&engine);
    windowDestroy(window);
    return 0;
}

#include "window.h"
#include "vulkan_engine.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

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

    VulkanEngine engine = {0};
    if (vulkanEngineInit(&engine) != 0)
    {
        windowDestroy(window);
        return 1;
    }

    vulkanEngineCleanup(&engine);
    windowDestroy(window);
    return 0;
}

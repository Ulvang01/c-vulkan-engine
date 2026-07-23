#include "window.h"
#include "vulkan_engine.h"

int main(void)
{
    GLFWwindow *window = windowCreate(640, 480, "BoogerAids");
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
#include "window.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

GLFWwindow *windowCreate(int width, int height, const char *title)
{
    if (!glfwInit())
    {
        LOG_ERROR("GLFW init failed");
        return NULL;
    }
    LOG_INFO("GLFW initialized OK");

    // No OpenGL context needed, Vulkan handles its own
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        LOG_ERROR("Window creation failed");
        glfwTerminate();
        return NULL;
    }
    LOG_INFO("Window created OK");
    return window;
}

void windowDestroy(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

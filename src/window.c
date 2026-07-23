#include "window.h"
#include <stdio.h>

GLFWwindow *windowCreate(int width, int height, const char *title)
{
    if (!glfwInit())
    {
        printf("GLFW init failed\n");
        return NULL;
    }
    printf("GLFW initialized OK\n");

    // No OpenGL context needed, Vulkan handles its own
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        printf("Window creation failed\n");
        glfwTerminate();
        return NULL;
    }
    printf("Window created OK\n");
    return window;
}

void windowDestroy(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

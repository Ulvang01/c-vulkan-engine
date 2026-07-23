#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

GLFWwindow* windowCreate(int width, int height, const char* title);
void windowDestroy(GLFWwindow* window);

#endif

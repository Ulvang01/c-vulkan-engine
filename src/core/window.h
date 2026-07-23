#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/**
 * @brief Creates a GLFW window configured for Vulkan rendering.
 *
 * Initializes GLFW and creates a window with no OpenGL context
 * (GLFW_CLIENT_API set to GLFW_NO_API), since Vulkan manages its
 * own rendering context.
 *
 * @param width  Window width in pixels.
 * @param height Window height in pixels.
 * @param title  Window title bar text.
 * @return Pointer to the created GLFWwindow, or NULL on failure.
 */
GLFWwindow *windowCreate(int width, int height, const char *title);

/**
 * @brief Destroys the window and terminates GLFW.
 * @param window The window to destroy.
 */
void windowDestroy(GLFWwindow *window);

#endif

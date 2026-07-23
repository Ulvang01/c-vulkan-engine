#ifndef SURFACE_H
#define SURFACE_H

#include "vulkan_engine.h"
#include <GLFW/glfw3.h>

/**
 * @brief Creates the Vulkan surface that connects the instance to the
 *        actual OS window, using GLFW's cross-platform helper instead
 *        of raw platform-specific APIs (Win32SurfaceCreateInfoKHR etc).
 * @param engine Pointer to the VulkanEngine to store the surface in.
 * @param window The GLFW window to create a surface for.
 * @return 0 on success, non-zero on failure.
 */
int createSurface(VulkanEngine *engine, GLFWwindow *window);

#endif
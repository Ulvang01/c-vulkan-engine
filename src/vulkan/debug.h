#ifndef DEBUG_H
#define DEBUG_H

#include "vulkan_engine.h"

/**
 * @brief Creates the debug messenger that routes validation layer output
 *        to debugCallback above.
 *
 * vkCreateDebugUtilsMessengerEXT is an extension function, not core Vulkan,
 * so it isn't directly linked, we have to look up its address at runtime
 * via vkGetInstanceProcAddr before we can call it.
 *
 * @param engine Pointer to the VulkanEngine structure to store the messenger in.
 * @return 0 on success, non-zero on failure.
 */
int setupDebugMessenger(VulkanEngine *engine);

/**
 * @brief Destroys the debug messenger created by setupDebugMessenger.
 *
 * @param engine Pointer to the VulkanEngine structure containing the messenger.
 */
void destroyDebugMessenger(VulkanEngine *engine);

#endif

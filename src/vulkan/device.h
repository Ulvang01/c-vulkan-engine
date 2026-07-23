#ifndef DEVICE_H
#define DEVICE_H

#include "vulkan_engine.h"

/**
 * @brief Enumerates all Vulkan-capable physical devices and picks the
 *        best-scoring one (see rateDeviceSuitability), storing it in
 *        the VulkanEngine structure.
 *
 * @param engine Pointer to the VulkanEngine structure to store the selected physical device.
 * @return 0 on success, non-zero on failure (no devices, or none suitable).
 */
int pickPhysicalDevice(VulkanEngine *engine);

/**
 * @brief Creates the logical device, which is the real handle used to
 *        issue commands to the GPU (unlike the physical device, which
 *        is just a description of the hardware).
 *
 * Also retrieves a handle to the graphics queue on that device, which
 * is what you'll submit rendering commands to later.
 *
 * @param engine Pointer to the VulkanEngine structure. Requires
 *               engine->physicalDevice to already be set (from
 *               pickPhysicalDevice).
 * @return 0 on success, non-zero on failure.
 */
int createLogicalDevice(VulkanEngine *engine);

#endif

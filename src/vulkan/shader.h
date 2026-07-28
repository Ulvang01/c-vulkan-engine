#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>

/**
 * @brief Reads a compiled SPIR-V file and wraps it in a VkShaderModule.
 *
 * The returned module is only needed during pipeline creation and should
 * be destroyed with vkDestroyShaderModule once the pipeline exists.
 *
 * @param device   Logical device the module is created on.
 * @param filepath Path to the .spv file, relative to the working directory.
 * @param module   Out-parameter, receives the created module on success.
 * @return 0 on success, non-zero on failure.
 */
int createShaderModule(VkDevice device, const char *filepath, VkShaderModule *module);

#endif
#ifndef PIPELINE_H
#define PIPELINE_H

#include "vulkan/vulkan_engine.h"

/**
 * @brief Creates the graphics pipeline: loads the compiled shaders and
 *        combines them with fixed-function state into a single immutable
 *        pipeline object.
 *
 * Shader modules are created and destroyed inside this function, the
 * pipeline retains its own compiled copy of the shader code, so the
 * modules serve no purpose once creation succeeds.
 *
 * @param engine Requires engine->device and engine->renderPass to be set.
 * @return 0 on success, non-zero on failure.
 */
int createGraphicsPipeline(VulkanEngine *engine);

#endif
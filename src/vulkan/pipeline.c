#include "pipeline.h"
#include "core/common.h"
#include "core/log.h"
#include "shader.h"
#include "vertex_buffer.h"
#include "vulkan_engine.h"

int createGraphicsPipeline(VulkanEngine *engine)
{
    // --- Shader modules ---
    // Local only. vkCreateGraphicsPipelines compiles what it needs into the
    // pipeline object, so these are destroyed before this function returns.

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    if (createShaderModule(engine->device, "shaders/triangle.vert.spv", &vertShaderModule) != 0)
        return 1;

    if (createShaderModule(engine->device, "shaders/triangle.frag.spv", &fragShaderModule) != 0)
    {
        vkDestroyShaderModule(engine->device, vertShaderModule, NULL);
        return 1;
    }

    // --- Shader stages ---
    // pName is the entry point inside the SPIR-V module. A module can hold
    // several entry points, ours both use "main".

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = { 0 };
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = { 0 };
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // --- Dynamic state ---
    // State listed here is NOT baked into the pipeline and must instead be
    // set per-frame during command buffer recording. Making viewport and
    // scissor dynamic means the pipeline survives a window resize: only the
    // swapchain, image views, and framebuffers need recreating, not this.

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = ARRAY_COUNT(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    // --- Vertex input ---
    // Zero bindings and attributes means "no vertex data to load". The three
    // triangle positions are currently a hardcoded array in the vertex shader,
    // indexed by gl_VertexIndex.
    // TODO: replace with real binding/attribute descriptions once vertex
    // buffers exist.

    VkVertexInputBindingDescription bindingDescription = vertexGetBindingDescription();
    VkVertexInputAttributeDescription attributeDescriptions[2];
    uint32_t attributeDescriptionCount = vertexGetAttributeDescriptions(attributeDescriptions);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // --- Input assembly ---
    // How incoming vertices are grouped into primitives. TRIANGLE_LIST means
    // every three vertices form one independent triangle, with no reuse
    // between them.
    // primitiveRestartEnable only applies to indexed draws with strip
    // topologies, irrelevant here.

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // --- Viewport state ---
    // Only the counts are baked in. pViewports/pScissors are deliberately
    // left null because both are dynamic state (see above), so Vulkan
    // ignores them here and reads the real values from the command buffer.

    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // --- Rasterizer ---
    // Turns triangles into fragments for the fragment shader.
    // FILL draws solid triangles; LINE gives wireframe but needs the
    // fillModeNonSolid device feature enabled.
    // Backface culling is ON, and front faces are declared clockwise. This
    // must match the vertex order in the shader or the triangle is silently
    // culled and nothing appears. Note CLOCKWISE is not Vulkan's default,
    // COUNTER_CLOCKWISE is the zero value.
    // lineWidth must be 1.0 unless the wideLines feature is enabled.
    // Depth bias is for shadow map acne, irrelevant without a depth buffer.

    VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // --- Multisampling ---
    // Disabled. rasterizationSamples must match the sample count on the
    // render pass color attachment, which is also 1.
    // Note this field is NOT safe to leave zero-initialized: VK_SAMPLE_COUNT_1_BIT
    // is 1, and a sample count of 0 is invalid.
    // Enabling MSAA later means changing this and the attachment together,
    // plus a resolve attachment, since you can't present a multisampled image
    // directly.

    VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // --- Color blending ---
    // Blending is off, so the fragment shader's output is written straight
    // through with no mixing against what's already in the framebuffer.
    // colorWriteMask is NOT affected by blendEnable and must be set: it
    // selects which channels are written at all, and leaving it zero silently
    // discards everything the fragment shader produces.
    // The blend factor and op fields are omitted deliberately, they're ignored
    // while blendEnable is FALSE.

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // --- Color blending state ---
    // Wraps the per-attachment blend config above. attachmentCount must match
    // the number of color attachments in the render pass, which is 1.
    // logicOp is a separate, mutually exclusive path from per-attachment
    // blending: enabling it makes the framebuffer write a bitwise operation
    // (AND, XOR, etc) instead of blending, and overrides blendEnable on every
    // attachment. Rarely used outside old-style 2D work.

    VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- Pipeline layout ---
    // Describes the layout of resources (uniform buffers, textures, etc) that
    // can be accessed by the pipeline. We don't have any yet, so this is empty.

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, NULL,
                               &engine->pipelineLayout) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create pipeline layout");
        vkDestroyShaderModule(engine->device, fragShaderModule, NULL);
        vkDestroyShaderModule(engine->device, vertShaderModule, NULL);
        return 1;
    }

    // --- The pipeline itself ---
    // Pulls every state struct above together, plus the render pass this
    // pipeline is compatible with. subpass is the index within that render
    // pass, we have exactly one, so 0.
    // basePipelineHandle/Index are for deriving a pipeline from an existing
    // similar one to speed up creation, unused here.

    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = ARRAY_COUNT(shaderStages);
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = engine->pipelineLayout;
    pipelineInfo.renderPass = engine->renderPass;
    pipelineInfo.subpass = 0;

    // The VK_NULL_HANDLE is a pipeline cache, which can persist compiled
    // pipelines across runs. Worth adding later if startup time matters.
    VkResult result = vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                                NULL, &engine->graphicsPipeline);

    vkDestroyShaderModule(engine->device, fragShaderModule, NULL);
    vkDestroyShaderModule(engine->device, vertShaderModule, NULL);

    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create graphics pipeline: %d", result);
        return 1;
    }

    LOG_INFO("Graphics pipeline created OK");
    return 0;
}
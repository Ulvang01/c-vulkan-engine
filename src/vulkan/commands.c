#include "commands.h"
#include "core/log.h"
#include "queue_families.h"
#include "vertex_buffer.h"
#include "vulkan_engine.h"

int createCommandPool(VulkanEngine *engine)
{
    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);

    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // RESET_COMMAND_BUFFER lets us re-record an individual buffer each frame.
    // Without it, the only way to reuse a buffer is resetting the whole pool.
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value;

    if (vkCreateCommandPool(engine->device, &poolInfo, NULL, &engine->commandPool) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create command pool");
        return 1;
    }

    LOG_INFO("Command pool created OK");
    return 0;
}

int createCommandBuffer(VulkanEngine *engine)
{
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = engine->commandPool;
    // PRIMARY can be submitted to a queue directly. SECONDARY can only be
    // called from a primary buffer, used for reusable command sequences.
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    VkCommandBuffer buffers[MAX_FRAMES_IN_FLIGHT];
    if (vkAllocateCommandBuffers(engine->device, &allocInfo, buffers) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to allocate command buffer");
        return 1;
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        engine->frames[i].commandBuffer = buffers[i];
    }

    LOG_INFO("Command buffer allocated OK");
    return 0;
}

int recordCommandBuffer(VulkanEngine *engine, VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to begin recording command buffer");
        return 1;
    }

    VkRenderPassBeginInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = engine->renderPass;
    renderPassInfo.framebuffer = engine->swapchainState.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){ 0, 0 };
    renderPassInfo.renderArea.extent = engine->swapchainState.extent;

    VkClearValue clearColor = { .color.float32 = { 0.0f, 0.0f, 0.0f, 1.0f } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, engine->graphicsPipeline);

    // Viewport and scissor are dynamic state, so they aren't baked into the
    // pipeline and must be set here every time the buffer is recorded.
    // Reading from swapchainState.extent is what lets a resize work without
    // touching the pipeline.
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)engine->swapchainState.extent.width;
    viewport.height = (float)engine->swapchainState.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = engine->swapchainState.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind the vertex buffer to binding 0, with no offset.
    VkBuffer vertexBuffers[] = { engine->mesh.vertices.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, engine->mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, engine->mesh.indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to record command buffer");
        return 1;
    }

    return 0;
}
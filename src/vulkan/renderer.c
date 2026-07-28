#include "renderer.h"
#include "commands.h"
#include "core/log.h"
#include "vulkan_engine.h"
#include <stdlib.h>

int createSyncObjects(VulkanEngine *engine)
{
    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL, &engine->imageAvailableSemaphore) !=
            VK_SUCCESS ||
        vkCreateFence(engine->device, &fenceInfo, NULL, &engine->inFlightFence) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create sync objects");
        return 1;
    }

    // One render-finished semaphore per swapchain image, not one total.
    // The fence only guarantees the submit completed, it says nothing about
    // whether the present that waits on this semaphore has finished, so a
    // single shared semaphore can be re-signalled while still in use.
    engine->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * engine->swapchain.imageCount);

    for (uint32_t i = 0; i < engine->swapchain.imageCount; i++)
    {
        if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL,
                              &engine->renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create render finished semaphore %u", i);
            return 1;
        }
    }

    LOG_INFO("Sync objects created OK");
    return 0;
}

int drawFrame(VulkanEngine *engine)
{
    // Wait for the previous frame to finish so the command buffer is free
    // to re-record. UINT64_MAX means no timeout.
    vkWaitForFences(engine->device, 1, &engine->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(engine->device, 1, &engine->inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(engine->device, engine->swapchain.swapchain, UINT64_MAX,
                          engine->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(engine->commandBuffer, 0);

    if (recordCommandBuffer(engine, engine->commandBuffer, imageIndex) != 0)
        return 1;

    // Wait on the acquire semaphore, but only at the stage that actually
    // writes color. Vertex shading can start before the image is ready.
    VkSemaphore waitSemaphores[] = { engine->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { engine->renderFinishedSemaphores[imageIndex] };

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &engine->commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // The fence is signalled once this submission completes on the GPU,
    // which is what the wait at the top of the next frame is waiting for.
    if (vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, engine->inFlightFence) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to submit draw command buffer");
        return 1;
    }

    VkSwapchainKHR swapchains[] = { engine->swapchain.swapchain };

    VkPresentInfoKHR presentInfo = { 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(engine->presentQueue, &presentInfo);

    return 0;
}
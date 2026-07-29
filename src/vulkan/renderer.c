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

    LOG_INFO("Sync objects created OK");
    return 0;
}

int drawFrame(VulkanEngine *engine, GLFWwindow *window)
{
    // Wait for the previous frame to finish so the command buffer is free
    // to re-record. UINT64_MAX means no timeout.
    vkWaitForFences(engine->device, 1, &engine->inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult =
        vkAcquireNextImageKHR(engine->device, engine->swapchain.swapchain, UINT64_MAX,
                              engine->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Acquire failed outright, so imageAvailableSemaphore was NOT
        // signalled. Safe to abandon this frame and retry next iteration.
        return recreateSwapchain(engine, window);
    }
    else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("Failed to acquire swapchain image: %d", acquireResult);
        return 1;
    }

    // Reset only once we know we are not returning early. Resetting above
    // the acquire check and then bailing would leave the fence unsignalled
    // with nothing queued to signal it, deadlocking the next frame's wait.
    vkResetFences(engine->device, 1, &engine->inFlightFence);

    vkResetCommandBuffer(engine->commandBuffer, 0);

    if (recordCommandBuffer(engine, engine->commandBuffer, imageIndex) != 0)
        return 1;

    // Wait on the acquire semaphore, but only at the stage that actually
    // writes color. Vertex shading can start before the image is ready.
    VkSemaphore waitSemaphores[] = { engine->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { engine->swapchain.renderFinishedSemaphores[imageIndex] };

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

    VkResult presentResult = vkQueuePresentKHR(engine->presentQueue, &presentInfo);

    // framebufferResized is checked too because some drivers never report
    // out-of-date on resize, the GLFW callback catches those.
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
        engine->framebufferResized)
    {
        engine->framebufferResized = false;
        return recreateSwapchain(engine, window);
    }
    else if (presentResult != VK_SUCCESS)
    {
        LOG_ERROR("Failed to present swapchain image: %d", presentResult);
        return 1;
    }

    return 0;
}
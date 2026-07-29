#include "renderer.h"
#include "commands.h"
#include "core/log.h"
#include "vulkan_engine.h"
#include <stdlib.h>

int createSyncObjects(VulkanEngine *engine)
{
    // Per-frame objects only. The renderFinished semaphores are sized by
    // swapchain image count, not frame count, so they live in swapchain.c.

    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Created already signalled, otherwise the first use of each slot would
    // wait forever on work that was never submitted
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        FrameData *frame = &engine->frames[i];

        if (vkCreateSemaphore(engine->device, &semaphoreInfo, NULL,
                              &frame->imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateFence(engine->device, &fenceInfo, NULL, &frame->inFlightFence) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create sync objects for frame %u", i);
            return 1;
        }
    }

    LOG_INFO("Sync objects created OK");
    return 0;
}

int drawFrame(VulkanEngine *engine, GLFWwindow *window)
{
    FrameData *frame = &engine->frames[engine->currentFrame];
    // Wait for this slot's previous submission to finish so its command
    // buffer is free to re-record. UINT64_MAX means no timeout.
    vkWaitForFences(engine->device, 1, &frame->inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult =
        vkAcquireNextImageKHR(engine->device, engine->swapchainState.swapchain, UINT64_MAX,
                              frame->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Acquire failed outright, so imageAvailableSemaphore was NOT
        // signalled and nothing was submitted. The slot is untouched, so
        // currentFrame deliberately does not advance, this frame is retried.
        return recreateSwapchain(engine, window);
    }
    else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("Failed to acquire swapchain image: %d", acquireResult);
        return 1;
    }

    // Reset only once we know we are not returning early. Resetting above
    // the acquire check and then bailing would leave the fence unsignalled
    // with nothing queued to signal it, deadlocking the next use of this slot.
    vkResetFences(engine->device, 1, &frame->inFlightFence);

    vkResetCommandBuffer(frame->commandBuffer, 0);

    if (recordCommandBuffer(engine, frame->commandBuffer, imageIndex) != 0)
        return 1;

    // Wait on the acquire semaphore, but only at the stage that actually
    // writes color. Vertex shading can start before the image is ready.
    VkSemaphore waitSemaphores[] = { frame->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    // Indexed by image, not by frame: the present waiting on this outlives
    // the submit, so a frame-indexed semaphore could be re-signalled while
    // an earlier present is still using it.
    VkSemaphore signalSemaphores[] = {
        engine->swapchainState.renderFinishedSemaphores[imageIndex]
    };

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame->commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // The fence is signalled once this submission completes on the GPU,
    // which is what the wait at the top of this slot's next turn waits for.
    if (vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, frame->inFlightFence) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to submit draw command buffer");
        return 1;
    }

    // Work is now queued on this slot, so move to the next one. Done here
    // rather than at the end of the function so the recreate path below
    // still advances, otherwise a resize would pin us to one slot.
    engine->currentFrame = (engine->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    VkSwapchainKHR swapchains[] = { engine->swapchainState.swapchain };

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
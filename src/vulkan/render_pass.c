#include "render_pass.h"
#include "core/log.h"
#include "vulkan_engine.h"

int createRenderPass(VulkanEngine *engine)
{
    VkAttachmentDescription colorAttachment = { 0 };
    colorAttachment.format = engine->swapchainState.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Layout the image should be in while the subpass is actually running,
    // distinct from the initial/final layouts above
    VkAttachmentReference colorAttachmentRef = { 0 };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult result =
        vkCreateRenderPass(engine->device, &renderPassInfo, NULL, &engine->renderPass);
    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create render pass: %d", result);
        return 1;
    }

    LOG_INFO("Render pass created OK");
    return 0;
}
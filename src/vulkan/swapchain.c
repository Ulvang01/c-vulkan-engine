#include "swapchain.h"
#include "core/log.h"
#include "queue_families.h"
#include "surface.h"
#include <stdlib.h>

/**
 * @brief Chooses the best surface format from the available options.
 */
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(SwapchainSupportDetails *details)
{
    for (uint32_t i = 0; i < details->formatCount; i++)
    {
        if (details->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            details->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return details->formats[i];
        }
    }
    // preferred format not found, first available is an acceptable fallback
    return details->formats[0];
}

/**
 * @brief Chooses the best present mode from the available options.
 */
static VkPresentModeKHR chooseSwapPresentMode(SwapchainSupportDetails *details)
{
    for (uint32_t i = 0; i < details->presentModeCount; i++)
    {
        if (details->presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return details->presentModes[i];
        }
    }
    // FIFO is guaranteed by the Vulkan spec to always be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 * @brief Chooses the best swap extent (resolution) for the swapchain.
 */
static VkExtent2D chooseSwapExtent(SwapchainSupportDetails *details, GLFWwindow *window)
{
    if (details->capabilities.currentExtent.width != UINT32_MAX)
    {
        return details->capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

    actualExtent.width = actualExtent.width < details->capabilities.minImageExtent.width
                             ? details->capabilities.minImageExtent.width
                             : actualExtent.width;
    actualExtent.width = actualExtent.width > details->capabilities.maxImageExtent.width
                             ? details->capabilities.maxImageExtent.width
                             : actualExtent.width;
    actualExtent.height = actualExtent.height < details->capabilities.minImageExtent.height
                              ? details->capabilities.minImageExtent.height
                              : actualExtent.height;
    actualExtent.height = actualExtent.height > details->capabilities.maxImageExtent.height
                              ? details->capabilities.maxImageExtent.height
                              : actualExtent.height;

    return actualExtent;
}

int querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface,
                          SwapchainSupportDetails *details)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details->capabilities);

    details->formats = NULL;
    details->formatCount = 0;

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

    if (formatCount != 0)
    {
        details->formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details->formats);
        details->formatCount = formatCount;
    }

    details->presentModes = NULL;
    details->presentModeCount = 0;

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

    if (presentModeCount != 0)
    {
        details->presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                                  details->presentModes);
        details->presentModeCount = presentModeCount;
    }

    return 0;
}

bool isSwapchainAdequate(SwapchainSupportDetails *details)
{
    return details->formatCount > 0 && details->presentModeCount > 0;
}

int createSwapchain(VulkanEngine *engine, GLFWwindow *window)
{
    SwapchainSupportDetails details = { 0 };
    querySwapchainSupport(engine->physicalDevice, engine->surface, &details);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(&details);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(&details);
    VkExtent2D extent = chooseSwapExtent(&details, window);

    // Request one more image than the strict minimum, so we're not stuck
    // waiting on the driver to free an image before we can acquire another
    uint32_t imageCount = details.capabilities.minImageCount + 1;

    // maxImageCount == 0 means "no upper limit", only clamp if there is one
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
    {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = engine->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // always 1, unless doing stereoscopic 3D
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value, indices.presentFamily.value };

    if (indices.graphicsFamily.value != indices.presentFamily.value)
    {
        // Graphics and present are different queue families, images need
        // to be usable across both without explicit ownership transfer
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        // Same family for both, exclusive is simpler and slightly faster,
        // no cross-queue synchronization needed
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result =
        vkCreateSwapchainKHR(engine->device, &createInfo, NULL, &engine->swapchain.swapchain);
    freeSwapchainSupportDetails(&details);

    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create swapchain: %d", result);
        return 1;
    }
    LOG_INFO("Swapchain created OK");

    // Retrieve the actual swapchain images, same two-call pattern as
    // everywhere else, the driver may have created more images than we
    // requested (imageCount was only a minimum), so re-query the real count
    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain.swapchain, &actualImageCount, NULL);

    engine->swapchain.images = malloc(sizeof(VkImage) * actualImageCount);
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain.swapchain, &actualImageCount,
                            engine->swapchain.images);
    engine->swapchain.imageCount = actualImageCount;

    engine->swapchain.imageFormat = surfaceFormat.format;
    engine->swapchain.extent = extent;

    return 0;
}

void freeSwapchainSupportDetails(SwapchainSupportDetails *details)
{
    if (details->formats)
    {
        free(details->formats);
        details->formats = NULL;
    }
    if (details->presentModes)
    {
        free(details->presentModes);
        details->presentModes = NULL;
    }
}

int createImageViews(VulkanEngine *engine)
{
    engine->swapchain.imageViews = malloc(sizeof(VkImageView) * engine->swapchain.imageCount);

    for (uint32_t i = 0; i < engine->swapchain.imageCount; i++)
    {
        VkImageViewCreateInfo createInfo = { 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = engine->swapchain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = engine->swapchain.imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(engine->device, &createInfo, NULL,
                              &engine->swapchain.imageViews[i]) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create image view %u", i);
            return 1;
        }
    }

    LOG_INFO("Created %u image views", engine->swapchain.imageCount);
    return 0;
}
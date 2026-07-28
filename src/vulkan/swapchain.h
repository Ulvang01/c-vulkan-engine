#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

// Forward declaration, we only need a pointer to this type here,
// not its full definition, avoids circular include with vulkan_engine.h
typedef struct VulkanEngine VulkanEngine;

/**
 * @brief The swapchain and everything derived from it.
 *
 * These fields share a lifetime: all of them are destroyed and recreated
 * together when the window is resized, which is why they live in one struct
 * rather than flat on VulkanEngine.
 *
 * @note images is owned by the swapchain itself. Only the array holding the
 *       handles is ours to free, the VkImages must not be destroyed
 *       individually. imageViews and framebuffers are ours and must be.
 */
typedef struct
{
    VkSwapchainKHR swapchain;
    VkImage *images;
    VkImageView *imageViews;
    VkFramebuffer *framebuffers;
    uint32_t imageCount;
    VkFormat imageFormat;
    VkExtent2D extent;
} SwapchainState;

/**
 * @brief What a given physical device and surface pairing supports.
 *
 * Purely descriptive, it reports options without judging them. Use
 * isSwapchainAdequate to turn it into a yes/no, and the chooser functions
 * in swapchain.c to pick concrete values from it.
 *
 * @warning Owns two heap allocations. Always pair with
 *          freeSwapchainSupportDetails.
 */
typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formatCount;
    VkPresentModeKHR *presentModes;
    uint32_t presentModeCount;
} SwapchainSupportDetails;

/**
 * @brief Queries surface capabilities, formats, and present modes for a
 *        device/surface pairing.
 *
 * Called twice in the engine's lifetime: once per candidate during device
 * scoring, and again on the chosen device during swapchain creation. The
 * result is short-lived both times.
 *
 * @param device  Physical device to query.
 * @param surface Surface to query against.
 * @param details Out-parameter, filled in on success. Caller must free it
 *                with freeSwapchainSupportDetails.
 * @return 0 on success, non-zero on failure.
 */
int querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface,
                          SwapchainSupportDetails *details);

/**
 * @brief Whether a device/surface pairing can support a usable swapchain.
 *
 * A device can report the VK_KHR_swapchain extension while still offering
 * zero formats or present modes for a particular surface, in which case no
 * swapchain can be created. Used as a hard disqualifier during device
 * scoring, not a scoring factor.
 *
 * @param details Result of a prior querySwapchainSupport call.
 * @return true if at least one format and one present mode are available.
 */
bool isSwapchainAdequate(SwapchainSupportDetails *details);

/**
 * @brief Frees the heap allocations owned by a SwapchainSupportDetails.
 *
 * Safe to call on a zero-initialized struct, and safe to call twice, the
 * pointers are nulled out.
 *
 * @param details The struct to release.
 */
void freeSwapchainSupportDetails(SwapchainSupportDetails *details);

/**
 * @brief Creates the swapchain and retrieves its images.
 *
 * Picks a surface format (sRGB preferred), present mode (mailbox preferred,
 * FIFO as the guaranteed fallback), and extent (the window's framebuffer
 * size, clamped to what the surface allows).
 *
 * @param engine Requires physicalDevice, device, and surface to be set.
 *               Populates engine->swapchain.
 * @param window Used to query the current framebuffer size.
 * @return 0 on success, non-zero on failure.
 */
int createSwapchain(VulkanEngine *engine, GLFWwindow *window);

/**
 * @brief Creates one image view per swapchain image.
 *
 * Swapchain images can't be used as render targets directly, a view is what
 * declares how to interpret one.
 *
 * @param engine Requires engine->swapchain to be populated by createSwapchain.
 * @return 0 on success, non-zero on failure.
 */
int createImageViews(VulkanEngine *engine);

/**
 * @brief Creates one framebuffer per image view, bound to the render pass.
 *
 * A framebuffer is what connects the render pass's abstract attachment
 * descriptions to concrete images for a given frame.
 *
 * @param engine Requires engine->renderPass and image views to already exist.
 * @return 0 on success, non-zero on failure.
 */
int createFramebuffers(VulkanEngine *engine);

#endif
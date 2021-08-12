#include "SwapChain.h"

#include "ImageUtils.h"

#include <array>
#include <stdexcept>

void SwapChain::Create(const GraphicsAPI& graphicsAPI, VkSurfaceKHR surface, size_t width, size_t height)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();

    CreateSwapChain(physicalDevice, logicalDevice, surface, width, height);
    CreateSwapChainImageViews(logicalDevice.GetVkDevice());
}

void SwapChain::CleanUp(const LogicalDevice& logicalDevice)
{
    VkDevice device = logicalDevice.GetVkDevice();

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void SwapChain::RecreateSwapChain(const GraphicsAPI& graphicsAPI, VkSurfaceKHR surface, size_t width, size_t height)
{
    CleanUp(graphicsAPI.GetLogicalDevice());

    Create(graphicsAPI, surface, width, height);
}

size_t SwapChain::GetImageCount() const
{
    return swapChainImages.size();
}

VkExtent2D SwapChain::GetExtent() const
{
    return swapChainExtent;
}

VkSurfaceKHR SwapChain::GetSurface() const
{
    return VkSurfaceKHR();
}

VkSwapchainKHR SwapChain::GetVkSwapChain() const
{
    return swapChain;
}

VkFormat SwapChain::GetImageFormat() const
{
    return swapChainImageFormat;
}

VkImage SwapChain::GetImage(size_t index) const
{
    return swapChainImages[index];
}

VkImageView SwapChain::GetImageView(size_t index) const
{
    return swapChainImageViews[index];
}

void SwapChain::CreateSwapChain(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, VkSurfaceKHR surface, size_t width, size_t height)
{
    SwapChainSupportDetails swapChainSupport = physicalDevice.GetSwapChainSupportDetails(surface);

    VkSurfaceFormatKHR  surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, width, height);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    QueueFamilyIndices indices = physicalDevice.GetQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkDevice device = logicalDevice.GetVkDevice();
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void SwapChain::CreateSwapChainImageViews(VkDevice device)
{
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++)
    {
        swapChainImageViews[i] = CreateSwapChainImageView(device, swapChainImages[i]);
    }
}

VkImageView SwapChain::CreateSwapChainImageView(VkDevice device, VkImage image)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    /*
    * Tutorial author views that triple buffering is a very nice trade-off.
    * It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images
    * that are as up-to-date as possible right until the vertical blank.
    * so, we look through the list to see if it's available:
    */
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, size_t width, size_t height)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent =
        {
              width,
              height
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
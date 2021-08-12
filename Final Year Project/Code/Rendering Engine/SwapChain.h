#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

class SwapChain
{
public:
    SwapChain() = default;
    ~SwapChain() = default;

    SwapChain(const SwapChain&) = delete;
    SwapChain(SwapChain&&) = delete;
    SwapChain& operator = (const SwapChain&) = delete;
    SwapChain& operator = (SwapChain&&) = delete;

    void Create(const GraphicsAPI& graphicsAPI, VkSurfaceKHR surface, size_t width, size_t height);
    void CleanUp(const LogicalDevice& logicalDevice);

    void RecreateSwapChain(const GraphicsAPI& graphicsAPI, VkSurfaceKHR surface, size_t width, size_t height);

    size_t GetImageCount() const;
    VkExtent2D GetExtent() const;

    VkSurfaceKHR GetSurface() const;
    VkSwapchainKHR GetVkSwapChain() const;

    VkFormat GetImageFormat() const;

    VkImage GetImage(size_t index) const;
    VkImageView GetImageView(size_t index) const;

private:
    void CreateSwapChain(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, VkSurfaceKHR surface, size_t width, size_t height);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, size_t width, size_t height);
    
    void CreateSwapChainImageViews(VkDevice device);
    VkImageView CreateSwapChainImageView(VkDevice device, VkImage image);
 
    VkSwapchainKHR swapChain;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

};


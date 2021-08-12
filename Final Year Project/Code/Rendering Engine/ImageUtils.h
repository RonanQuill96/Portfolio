#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "CommandPool.h"

#include <stdexcept>

struct Image
{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VkDescriptorImageInfo GetImageInfo(VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
    {
        VkDescriptorImageInfo imageInfo{};

        imageInfo.imageLayout = layout;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = sampler;

        return imageInfo;
    }

    void CleanUp(VkDevice device)
    {
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);
    }
};


struct Texture
{
    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation allocation;
    VmaAllocator m_allocator;

    VkDescriptorImageInfo GetImageInfo(VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
    {
        VkDescriptorImageInfo imageInfo{};

        imageInfo.imageLayout = layout;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = sampler;

        return imageInfo;
    }

    void CleanUp(VkDevice device)
    {
        vkDestroyImageView(device, textureImageView, nullptr);
        vmaDestroyImage(m_allocator, textureImage, allocation);
    }
};

struct CubeMapLoadInfo
{
    std::string folder;

    std::string right;
    std::string left;
    std::string top;
    std::string bottom;
    std::string front;
    std::string back;
};


struct CubeMapSaveInfo
{
    std::string folder;
    VkFormat imageFormat;

    int width;
    int height;
    int channels;
    int mipLevels;
};

struct ImageSaveInfo
{
    std::string file;

    int width;
    int height;
    int channels;
};

class ImageUtils
{
public:

    static Image LoadCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapLoadInfo& cubeMapLoadInfo);
    static void SaveCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapSaveInfo& cubeMapSaveInfo, const Image& image);
    
    static void SaveImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const ImageSaveInfo& imageSaveInfo, const Image& image, VkFormat imageFormat);

    static bool HasStencilComponent(VkFormat format);

    static VkImageView CreateImageView(const LogicalDevice& logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkDevice device = logicalDevice.GetVkDevice();
        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }
};
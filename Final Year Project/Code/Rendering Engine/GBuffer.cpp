#include "GBuffer.h"

#include <stdexcept>

void GBuffer::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportExtent)
{
    extent = viewportExtent;

    normals = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);
    albedo = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);
    metalicRoughness = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);

    depth = CreateLayer(graphicsAPI, commandPool, graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, viewportExtent);

    CreateSampler(graphicsAPI);
}

void GBuffer::CleanUp(VkDevice device)
{
    normals.image.CleanUp(device);
    albedo.image.CleanUp(device);
    metalicRoughness.image.CleanUp(device);
    depth.image.CleanUp(device);

    vkDestroySampler(device, gBufferSampler, nullptr);
}

VkAttachmentDescription GBuffer::GetNormalsLayerAttachmentDesciption() const
{
    VkAttachmentDescription normalsLayer{};

    normalsLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    normalsLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalsLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalsLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalsLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalsLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalsLayer.format = normals.format;
    normalsLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return normalsLayer;
}

VkAttachmentDescription GBuffer::GetAlbedoLayerAttachmentDesciption() const
{
    VkAttachmentDescription albedoLayer{};

    albedoLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    albedoLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    albedoLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    albedoLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    albedoLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    albedoLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    albedoLayer.format = albedo.format;
    albedoLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return albedoLayer;
}

VkAttachmentDescription GBuffer::GetMetalicRougnessLayerAttachmentDesciption() const
{
    VkAttachmentDescription metalicRoughnessLayer{};

    metalicRoughnessLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    metalicRoughnessLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    metalicRoughnessLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    metalicRoughnessLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    metalicRoughnessLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    metalicRoughnessLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    metalicRoughnessLayer.format = metalicRoughness.format;
    metalicRoughnessLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return metalicRoughnessLayer;
}

VkAttachmentDescription GBuffer::GetDepthLayerAttachmentDesciption() const
{
    VkAttachmentDescription depthLayer{};

    depthLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    depthLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthLayer.format = depthLayer.format;
    depthLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return depthLayer;
}

VkSampler GBuffer::GetSampler() const
{
    return gBufferSampler;
}

GBuffer::Layer GBuffer::CreateLayer(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkFormat format, VkImageUsageFlagBits usage, VkExtent2D viewportExtent)
{
    Layer layer;

    layer.format = format;

    VkImageAspectFlags aspectMask = 0;
    VkImageLayout imageLayout;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = viewportExtent.width;
    imageInfo.extent.height = viewportExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &layer.image.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, layer.image.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &layer.image.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, layer.image.textureImage, layer.image.textureImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = layer.image.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &layer.image.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return layer;
}

void GBuffer::CreateSampler(const GraphicsAPI& graphicsAPI)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();
    if (vkCreateSampler(device, &samplerInfo, nullptr, &gBufferSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

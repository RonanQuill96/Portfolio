#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"
#include "CommandPool.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GraphicsAPI.h"
#include "RenderingEngine.h"

#include "ImageUtils.h"

class RadianceMapGenerationPass
{
public:
    Image GenerateRadianceMap(RenderingEngine& renderingEngine, VkImageView environmentMap, VkExtent2D textureDimensions, size_t mipLevels);

private:
    Image CreateRadianceMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, size_t mipCount);
    void CreateFrameBufferImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, size_t mipCount);

    void CreateRenderPass(const LogicalDevice& logicalDevice, Image& radianceMap, VkExtent2D textureDimension, size_t mipCount);

    void CreateDescriptorSetLayout(const LogicalDevice& logicalDevice);
    void CreateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
    void CreatePipeline(const LogicalDevice& logicalDevice, Image& radianceMap, VkExtent2D textureDimension);
    void CreateDescriptorSet(const LogicalDevice& logicalDevice, VkDescriptorPool descriptorPool, VkImageView environmentMap);
    void CreateSampler(const LogicalDevice& logicalDevice);

    void RenderRadianceMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, Image& radianceMap, size_t mipCount);

    VkRenderPass renderPass;


    struct FrameBufferData
    {
        VkFramebuffer frameBuffer;

        VkImage frameBufferImage;
        VkDeviceMemory frameBufferImageMemory;
        VkImageView frameBufferImageView;
    };

    std::vector<FrameBufferData> frameBuffers;
    
    //FrameBufferAttachment color, depth;

    VkSampler sampler;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
};


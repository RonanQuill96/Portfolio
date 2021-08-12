#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"
#include "CommandPool.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GraphicsAPI.h"
#include "Vertex.h"

#include "RenderingEngine.h"

#include "ImageUtils.h"

class IntegrationMapGeneratorPass
{
public:
    Image GenerateIntegrationMap(RenderingEngine& renderingEngine, VkExtent2D textureDimensions);

private:
    Image CreateIntegrationMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);
    void CreateFrameBufferImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);

    void CreateRenderPass(const LogicalDevice& logicalDevice, Image& irradianceMap, VkExtent2D textureDimension);

    void CreateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
    void CreatePipeline(const LogicalDevice& logicalDevice, Image& irradianceMap, VkExtent2D textureDimension);

    void RenderIntegrationMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, Image& irradianceMap);

    VkRenderPass renderPass;

    VkFramebuffer frameBuffer;

    VkImage frameBufferImage;
    VkDeviceMemory frameBufferImageMemory;
    VkImageView frameBufferImageView;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    //VkDescriptorSetLayout descriptorSetLayout;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
};


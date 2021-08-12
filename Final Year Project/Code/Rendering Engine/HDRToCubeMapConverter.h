#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"

#include "ImageUtils.h"
#include "ImageData.h"

#include "LogicalDevice.h"
#include "CommandPool.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GraphicsAPI.h"
#include "DescriptorSetManager.h"

#include <string>

class HDRToCubeMapConverter
{
public:
	Image ConvertHDRToCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, std::string filename, VkExtent2D textureDimensions);

private:
    Image CreateHDRImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, ImageData& hdrImageData);
    Image CreateCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);
    void CreateFrameBufferImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);

    void CreateRenderPass(const LogicalDevice& logicalDevice);
    void CreateRenderTargets(const LogicalDevice& logicalDevice, VkExtent2D textureDimension);

    void CreateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
    void CreatePipeline(const LogicalDevice& logicalDevice, VkExtent2D textureDimension);

    void RenderCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, Image& cubeMap, Image& hdrImage);

    DescriptorSetManager<HDRToCubeMapConverter*, 1, false> descriptorSetManager;

    VkRenderPass renderPass;

    VkFramebuffer frameBuffer;

    VkImage frameBufferImage;
    VkDeviceMemory frameBufferImageMemory;
    VkImageView frameBufferImageView;

    VkSampler sampler;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    size_t indexCount = 0;
};


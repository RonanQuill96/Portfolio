#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"
#include "CommandPool.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GraphicsAPI.h"
#include "Vertex.h"

#include "ImageUtils.h"

#include <array>


#include "RenderingEngine.h"

class IrrandianceMapPass
{
public:
    Image GenerateIrradainceMap(RenderingEngine& renderingEngine, VkImageView environmentMap, VkExtent2D textureDimensions);

private:
    Image CreateIrradianceMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);
    void CreateFrameBufferImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension);

	void CreateRenderPass(const LogicalDevice& logicalDevice, Image& irradianceMap, VkExtent2D textureDimension);

	void CreateDescriptorSetLayout(const LogicalDevice& logicalDevice);
    void CreateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
	void CreatePipeline(const LogicalDevice& logicalDevice, Image& irradianceMap, VkExtent2D textureDimension);
    void CreateDescriptorSet(const LogicalDevice& logicalDevice, VkDescriptorPool descriptorPool, VkImageView environmentMap);
	void CreateSampler(const LogicalDevice& logicalDevice);

    void RenderIrradianceMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, Image& irradianceMap);

    VkRenderPass renderPass;

	VkFramebuffer frameBuffer;

    VkImage frameBufferImage;
    VkDeviceMemory frameBufferImageMemory;
    VkImageView frameBufferImageView;
	//FrameBufferAttachment color, depth;

	VkSampler sampler;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
};


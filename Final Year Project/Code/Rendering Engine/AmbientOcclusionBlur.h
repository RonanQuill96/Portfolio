#pragma once

#include "VulkanIncludes.h"

#include "DescriptorSetManager.h"
#include "ImageUtils.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

struct AOBlurParameters
{
	glm::vec2 AORes;
	glm::vec2 InvAORes;

	bool horizontal;
};

class AmbientOcclusionBlur
{
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportSize, Image& aoMap);

	void BlurAOMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& aoMap);

private:
	void CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
	void CreatePingPongImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
	void CreateRenderPass(const GraphicsAPI& graphicsAPI, Image& aoMap);
	void CreatePipeline(const GraphicsAPI& graphicsAPI);

	void BlurX(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& aoMap);
	void BlurY(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer);


	VkFramebuffer aoMapBuffer;
	VkFramebuffer pingPongBuffer;

	Image pingPongImage;

	VkRenderPass renderPass;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	DescriptorSetManager< AmbientOcclusionBlur* > blurXdescriptorSetManager;
	DescriptorSetManager< AmbientOcclusionBlur* > blurYdescriptorSetManager;

	VertexBuffer fullscreenQuadVertices;
	IndexBuffer fullscreenQuadIndices;

	VkExtent2D aoMapSize;
};


#pragma once

#include "VulkanIncludes.h"

#include <imgui.h>

#include "GraphicsAPI.h"
#include "CommandPool.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ImageUtils.h"

#include "DescriptorSetManager.h"

class UIRenderer
{
public:
	struct PushConstants
	{
		glm::vec2 scale;
		glm::vec2 translation;
	};

public:
	void Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent, CommandPool& commandPool, VkRenderPass renderPass);

	void UpdateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);

	void DrawCurrentFrame(VkCommandBuffer commandBuffer);

	void CleanUp();

private:
	VkSampler sampler;

	VertexBuffer vertexBuffer;
	size_t vertexCount = 0;
	IndexBuffer indexBuffer;
	size_t indexCount = 0;

	Image font;

	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	VkDevice m_device;

	DescriptorSetManager<UIRenderer*, 1, false> descriptorSetManager;
};


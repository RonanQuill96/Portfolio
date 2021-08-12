#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"
#include "LogicalDevice.h"
#include "GBuffer.h"

#include "DescriptorSetManager.h"

class DirectionalLightComponent;

class DirectionalLightPipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout);
	void CleanUp(VkDevice device);

	void Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, const DirectionalLightComponent* directionalLight, VkDescriptorSet perLightingPassDescriptorSet);
private:

	VkDescriptorSet CreateDirectionalLightDescriptorSet(const DirectionalLightComponent* directionalLight);

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout perDirectionalLightDescriptorSetLayout;

	DescriptorSetManager<DirectionalLightComponent*> directionalLightComponentDescriptorSetManager;
};


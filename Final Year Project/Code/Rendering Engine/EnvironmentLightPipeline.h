#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"
#include "LogicalDevice.h"

#include "DescriptorSetManager.h"

#include "ImageUtils.h"

class EnvironmentMap;

class EnvironmentLightPipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout);
	void CleanUp(VkDevice device);

	void Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, EnvironmentMap* environmentMap,  VkDescriptorSet perLightingPassDescriptorSet, Image& aoMap, VkSampler pointSampler);
private:

	VkDescriptorSet CreateEnvironmentMapDescriptorSet(EnvironmentMap* environmentMap);

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	DescriptorSetManager<EnvironmentMap*> environmentMapDescriptorSetManager;

	VkDescriptorSetLayout perEniromentMapSetLayout;
	VkDescriptorSet perEniromentMapDS;

	DescriptorSetManager<EnvironmentLightPipeline*> ambientOcclusionMapDescriptorSetManager;
	VkDescriptorSetLayout ambientOcclusionMapSetLayout;

	VkSampler enviromentMapSampler;
	VkSampler integrationMapSampler;
};


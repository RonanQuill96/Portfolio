#pragma once

#include "VulkanIncludes.h"

#include "GBuffer.h"
#include "DirectionalLightPipeline.h"
#include "EnvironmentLightPipeline.h"
#include "SwapChain.h"
#include "GraphicsAPI.h"
#include "LogicalDevice.h"
#include "CommandPool.h"
#include "Scene.h"
#include "ClusteredLightCulling.h"

struct PerLightingPassInfo
{
	glm::vec3 camPosition;
	float pad1;
	glm::mat4 inverseProjection;
	glm::mat4 inverseView;
};

class LightingPass
{
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool,  const SwapChain& swapChain, VkRenderPass renderPass, const GBuffer& gBuffer);
	void CleanUp(VkDevice device);

	void Render(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo, VkExtent2D viewportExtent, Image& aoMap, VkSampler pointSampler);
	void PerformLightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D viewportExtent);

	ClusteredLightCulling clusteredLightCulling;
private:
	void CreateDescriptors(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer);
	void CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);

	DirectionalLightPipeline directionalLightPipeline;
	EnvironmentLightPipeline environmentLightPipeline;

	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;

	UniformBuffer<PerLightingPassInfo> perLightingPassInfoBuffer;

	VkDescriptorSetLayout perLightingPassDescriptorSetLayout;

	DescriptorSetManager<LightingPass*, 1, false> perLightingPassDescriptorSetManager;
	VkDescriptorSet perLightingPassDescriptorSet;
};


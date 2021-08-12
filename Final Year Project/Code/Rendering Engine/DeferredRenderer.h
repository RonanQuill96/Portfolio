#pragma once

#include "VulkanIncludes.h"

#include "GeometryPass.h"
#include "LightingPass.h"

#include "GBuffer.h"

class DeferredRenderer
{
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain, VkRenderPass renderPass);
	void CleanUp(VkDevice device);

	VkCommandBuffer RenderGBuffer(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene);
	VkCommandBuffer PerformLightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D extent);
	
	VkCommandBuffer PerformLightingPass(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo, VkExtent2D extent, Image& aoMap);

	GBuffer gBuffer;
	LightingPass lightingPass;
private:

	GeometryPass geometryPass;
};


#include "DeferredRenderer.h"

void DeferredRenderer::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain, VkRenderPass renderPass)
{
	gBuffer.Create(graphicsAPI, commandPool, swapChain.GetExtent());
	geometryPass.Create(graphicsAPI, gBuffer);
	lightingPass.Create(graphicsAPI, commandPool, swapChain, renderPass, gBuffer);
}

void DeferredRenderer::CleanUp(VkDevice device)
{
	gBuffer.CleanUp(device);
	geometryPass.CleanUp(device);
	lightingPass.CleanUp(device);
}

VkCommandBuffer DeferredRenderer::RenderGBuffer(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene)
{
	geometryPass.Render(logicalDevice, commandPool, commandBuffer, gBuffer, scene);

	return commandBuffer;
}

VkCommandBuffer DeferredRenderer::PerformLightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D extent)
{
	lightingPass.PerformLightingCull(commandBuffer, scene, extent);
	return commandBuffer;
}

VkCommandBuffer DeferredRenderer::PerformLightingPass(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo, VkExtent2D extent, Image& aoMap)
{
	lightingPass.Render(commandBuffer, logicalDevice, commandPool, scene, commandBufferInheritanceInfo, extent, aoMap, gBuffer.gBufferSampler);
	return commandBuffer;
}

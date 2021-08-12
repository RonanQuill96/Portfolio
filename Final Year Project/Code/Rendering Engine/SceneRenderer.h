#pragma once

#include "VulkanIncludes.h"

#include "Scene.h"

#include "GraphicsAPI.h"

#include "DeferredRenderer.h"
#include "ForwardRenderer.h"
#include "UIRenderer.h"

class SceneRenderer
{
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain);
	void OnSwapChainChanged(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain);

	void Prepare(const Scene& scene);

	void CleanUp(VkDevice device);

	void UpdateUI(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);

	void GenerateGBuffer(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene,VkExtent2D swapChainExtent, size_t imageIndex);
	void LightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D swapChainExtent);
	void PerformShading(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, VkExtent2D swapChainExtent, size_t imageIndex);
	 
	void RenderScene(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, VkExtent2D swapChainExtent, size_t imageIndex);

	std::vector<Image> renderImages;

	std::vector<VkImageView> depthImageViews;

	DeferredRenderer deferredRenderer;
private:
	void CreateDepthBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain);
	void CreateFrameBuffers(const LogicalDevice& logicalDevice, const SwapChain& swapChain);
	void CreateRenderPass(const LogicalDevice& logicalDevice, const PhysicalDevice& physicalDevice, const SwapChain& swapChain);
	void CreateSyncObjects();
	
	void CreateRenderImages(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain);

	VkRenderPass sceneRenderPass;
	std::vector<VkFramebuffer> framebuffers;

	std::vector<VkImage> depthImages;
	std::vector<VkDeviceMemory> depthImagesMemory;

	ForwardRenderer forwardRenderer;
	UIRenderer uiRenderer;


};


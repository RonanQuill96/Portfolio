#pragma once

#include "VulkanIncludes.h"

#include "StaticMeshPipeline.h"
#include "StaticMeshPBRTexturePipeline.h"

#include "GBuffer.h"

class GeometryPass
{
public:
	void Create(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer);
	void CleanUp(VkDevice device);

	void Render(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const GBuffer& gBuffer, const Scene& scene);

private:
	void CreateRenderPass(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer);
	void CreateFrameBuffer(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer);

	VkRenderPass renderPass;
	VkFramebuffer gBufferFrameBuffer;

	StaticMeshPBRMaterialsPipeline staticMeshPipeline;
	StaticMeshPBRTexturePipeline staticMeshPBRTexturePipeline;
};


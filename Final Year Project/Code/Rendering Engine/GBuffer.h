#pragma once

#include "ImageUtils.h"

#include "VulkanIncludes.h"
#include "GraphicsAPI.h"
#include "CommandPool.h"

struct GBuffer
{
public:
	struct Layer
	{
		Image image;
		VkFormat format;
	};
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportExtent);
	void CleanUp(VkDevice device);

	VkAttachmentDescription GetNormalsLayerAttachmentDesciption() const;
	VkAttachmentDescription GetAlbedoLayerAttachmentDesciption() const;
	VkAttachmentDescription GetMetalicRougnessLayerAttachmentDesciption() const;
	VkAttachmentDescription GetDepthLayerAttachmentDesciption() const;

	VkSampler GetSampler() const;

private:
	Layer CreateLayer(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkFormat format, VkImageUsageFlagBits usage, VkExtent2D viewportExtent);

	void CreateSampler(const GraphicsAPI& graphicsAPI);

public:
	Layer normals;
	Layer albedo;
	Layer metalicRoughness;

	Layer depth;

	VkExtent2D extent;

	VkSampler gBufferSampler;
};

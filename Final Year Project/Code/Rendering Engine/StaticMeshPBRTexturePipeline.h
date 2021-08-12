#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"

#include "Scene.h"

#include "GBuffer.h"

#include "MeshMaterial.h"

#include "DescriptorSetManager.h"

class MeshComponent;

class StaticMeshPBRTexturePipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkRenderPass geometryRenderPass, VkExtent2D viewportExtent);
	void CleanUp(VkDevice device);

	void Render(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial);

private:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayoutMesh;
	VkDescriptorSetLayout descriptorSetLayoutMaterial;

	DescriptorSetManager<const MeshComponent*> descriptorSetManagerMesh;
	DescriptorSetManager<Material*> descriptorSetManagerMaterial;

	VkSampler sampler;
};


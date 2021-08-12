#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"

#include "Scene.h"

#include "GBuffer.h"

#include "MeshMaterial.h"

#include "DescriptorSetManager.h"

#include "DescriptorSetLayoutBuilder.h"

class StaticMeshPBRMaterialsPipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkRenderPass geometryRenderPass, VkExtent2D viewportExtent);
	void CleanUp(VkDevice device);

	void Render(VkCommandBuffer commandBuffer, const MeshComponent* mesh, const MeshMaterial& meshMaterial);
private:
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	DescriptorSetManager<const MeshComponent*> meshDescriptorSetManager;
	VkDescriptorSetLayout meshDescriptorSetLayout;

	DescriptorSetManager<Material*> materialDescriptorSetManager;
	VkDescriptorSetLayout materialDescriptorSetLayout;
};


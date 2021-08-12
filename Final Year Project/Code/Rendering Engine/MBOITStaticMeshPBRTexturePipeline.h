#pragma once

#include "VulkanIncludes.h"

#include "DescriptorSetManager.h"

#include "ImageUtils.h"
#include "SwapChain.h"

#include "Material.h"

#include "MeshComponent.h"

#include "EnvironmentMap.h"

#include "CameraComponent.h"

#include "MBOITUtils.h"

class MBOITStaticMeshPBRTexturePipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass absorbanceRenderPass, VkRenderPass transmittanceRenderPass, VkDescriptorSetLayout perTransmittancePassDescriptorSetLayout);

	void RenderAbsorbance(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial, MBOITPushConstants mboitPushConstants, VkSampler sampler);
	
	void RenderTransmittanceClustered(VkCommandBuffer commandBuffer,
		const MeshComponent* meshComponent,
		const MeshMaterial& meshMaterial,
		MBOITTransmittanceClusteredConstants mboitTransmittanceConstants,
		VkDescriptorSet perTransmittancePassDescriptor, VkSampler sampler);

private:
	void CreateAbsorbancePipeline(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass absorbanceRenderPass);
	void CreateTrasnmitanceClusteredPipeline(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass transmittanceRenderPass, VkDescriptorSetLayout perTransmittancePassDescriptorSetLayout);

	VkPipeline absorbancePipeline;
	VkPipelineLayout absorbancePipelineLayout;

	VkDescriptorSetLayout absorbanceDescriptorSetLayoutMesh;
	DescriptorSetManager<const MeshComponent*> absorbanceDescriptorSetManagerMesh;

	VkDescriptorSetLayout absorbanceDescriptorSetLayoutMaterial;
	DescriptorSetManager<Material*> absorbanceDescriptorSetManagerMaterial;

	VkDescriptorSetLayout absorbanceDescriptorSetLayoutClusters;
	DescriptorSetManager<MBOITStaticMeshPBRTexturePipeline*> absorbanceDescriptorSetManagerClusters;

	VkPipeline transmittanceClusteredPipeline;
	VkPipelineLayout transmittanceClusteredPipelineLayout;

	VkDescriptorSetLayout perMeshDescriptorSetLayout;
	DescriptorSetManager<const MeshComponent*> perMeshDescriptorSetManager;

	VkDescriptorSetLayout perMaterialDescriptorSetLayout;
	DescriptorSetManager<Material*> perMaterialDescriptorSetManager;
};


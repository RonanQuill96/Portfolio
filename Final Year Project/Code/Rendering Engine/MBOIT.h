#pragma once

#include "VulkanIncludes.h"

#include "ImageUtils.h"
#include "SwapChain.h"

#include "Material.h"
#include "DescriptorSetManager.h"

#include "MBOITUtils.h"

#include "MBOITStaticMeshPBRParametersPipeline.h"
#include "MBOITStaticMeshPBRTexturePipeline.h"

#include "Scene.h"

#include "UniformBuffer.h"
#include "ClusteredCullingUtil.h"
#include "ClusteredLightCulling.h"

class MBOIT
{
public:
	void Create(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews, VkRenderPass compositeRenderPass);
	void Prepare(const Scene& scene);
	void RenderSceneAbsorbance(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const Scene& scene, size_t imageIndex);
	
	void RenderSceneTransmittanceClustered(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const Scene& scene, size_t imageIndex, const UniformBuffer<PointLightInfo>& pointLightInfoBuffer, const StorageBuffer<CulledLightsPerCluster>& culledLightsPerCluster,
		size_t cluster_size_x, size_t cluster_x_count, size_t cluster_y_count, size_t cluster_z_slices);
	
	void CompositieScene(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& opaqueMeshes, size_t imageIndex);

	VkSampler enviromentMapSampler;
	VkSampler integrationMapSampler;
	VkSampler sampler;

	static float overestimation;
	static float bias;
private:
	void CreateAbsorancePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews);
	void CreateTransmittancePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews);
	void CreateCompositePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, VkRenderPass compositeRenderPass);

	void CreateSamplers(const LogicalDevice& logicalDevice);

	VkDescriptorSetLayout trasnmittancePassDescriptorSetLayout;
	DescriptorSetManager<size_t> trasnmittancePassClusteredDescriptorSetManager;

	VkRenderPass absoranceRenderPass;
	std::vector<VkFramebuffer> absoranceFrameBuffer;
	VkExtent2D extent;

	VkFormat absorbanceImageFormat;

	std::vector<Image> b1234;
	std::vector<Image> b56;
	std::vector<Image> b0;

	VkRenderPass transmittanceRenderPass;
	std::vector < VkFramebuffer > transmittanceFrameBuffer;

	MBOITStaticMeshPBRParametersPipeline mboitStaticMeshPBRParametersPipeline;
	MBOITStaticMeshPBRTexturePipeline mboitStaticMeshPBRTexturePipeline;

	UniformBuffer<MBOITPerFrameInfo> perFrameInfoBuffer;

	std::vector<Image> transparentMeshes;

	AABB transparentMeshBounds;

	Image CreateImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkFormat format, VkImageUsageFlagBits usage, VkExtent2D viewportExtent);
	
	float lnDepthMin;
	float lnDepthMax;

	VkPipeline compositePipeline;
	VkPipelineLayout compositePipelineLayout;
	VkDescriptorSetLayout compositeDescriptorSetLayout;
	DescriptorSetManager<size_t> compositeDescriptorSetManager;

	VertexBuffer compositeVertexBuffer;
	IndexBuffer compositeIndexBuffer;
};


#pragma once

#include "VulkanIncludes.h"

#include "GBuffer.h"

#include "DescriptorSetManager.h"
#include "StorageBuffer.h"
#include "UniformBuffer.h"
#include "GraphicsAPI.h"
#include "CameraComponent.h"
#include "PointLightComponent.h"

#include "MeshComponent.h"

#include "ClusteredCullingUtil.h"

#define MAX_POINT_LIGHT_PER_CLUSTER 1023
struct CulledLightsPerCluster
{
	uint32_t count;
	uint32_t lightindices[MAX_POINT_LIGHT_PER_CLUSTER];
};

struct Cluster
{
	glm::vec4 minVertex;
	glm::vec4 maxVertex;
};

struct PLPushObjects
{
	glm::ivec3 clusterCounts;
	int clusterSizeX;

	glm::mat4 inverseViewProjection;

	glm::vec3 cameraPosition;
	float pad;

	float zNear;
	float zFar;

	float scale;
	float bias;
};

class ClusteredLightCulling
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent);

	void CreateClusters(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, VkExtent2D viewportExtent);

	void CreatePointLightShader(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout);

	void MarkActiveClusters(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, const GBuffer& gBuffer, VkExtent2D viewportExtent);
	void CullLights(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, const std::vector<PointLightComponent*>* pointLights, VkExtent2D viewportExtent);
	void Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, VkDescriptorSet perLightingPassDescriptorSet, CameraComponent* cameraComponent, VkExtent2D viewportExtent);

	static StorageBuffer<CulledLightsPerCluster> culledLightsPerCluster;
	static StorageBuffer<Cluster> clusters;

	static UniformBuffer<PointLightInfo> pointLightInfoBuffer;

	static constexpr size_t cluster_size_x = 32;
	static constexpr size_t cluster_size_y = 32;

	static size_t cluster_x_count;
	static size_t cluster_y_count;
	static constexpr size_t cluster_z_slices = 16;
	size_t cluster_count;

private:
	void CreateMarkActiveClustersPipeline(const GraphicsAPI& graphicsAPI);

	VkPipeline cullingPipeline;
	VkPipelineLayout cullingPipelineLayout;

	VkPipeline createClustersPipeline;
	VkPipelineLayout createClustersPipelineLayout;
	VkDescriptorSetLayout createClustersDescriptorSetLayout;
	DescriptorSetManager<ClusteredLightCulling*, 1, false> createClustersDescriptorSetManager;

	VkDescriptorSetLayout perPointLightDescriptorSetLayout;

	DescriptorSetManager<ClusteredLightCulling*, 1, false> clusteredPointLightsDescriptorSetManagerCulling;

	VkPipeline markActiveClustersPipeline;
	VkPipelineLayout markActiveClustersPipelineLayout;
	VkDescriptorSetLayout markActiveClustersDescriptorSetLayout;
	DescriptorSetManager<ClusteredLightCulling*, 1, false> markActiveClustersDescriptorSetManager;

	VkPipeline fragPipeline;
	VkPipelineLayout fragPipelineLayout;

	DescriptorSetManager<ClusteredLightCulling*, 1, false> clusteredPointLightsDescriptorSetManagerFrag;
};


#pragma once

#include "VulkanIncludes.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include "DescriptorSetManager.h"

#include "GBuffer.h"

#include "ImageUtils.h"

#include "UniformBuffer.h"

#include "AmbientOcclusionBlur.h"

struct HBAOParameters
{
	glm::vec2 AORes;
	glm::vec2 InvAORes;

	glm::vec2 NoiseScale;
	float AOStrength;
	float R;

	float R2;
	float NegInvR2;
	float TanBias;
	float MaxRadiusPixels;

	int NumDirections;
	int NumSamples;
	float pad1;
	float pad2;
};

struct HBAOPushConstantObject
{
	glm::mat4 inverseProjection;

	glm::vec2 FocalLen;
	glm::vec2 UVToViewA;
	glm::vec2 UVToViewB;

	glm::vec2 LinMAD;
};

class HorizonBasedAmbientOcclusion
{
public:
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportSize);

	void RenderAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, const GBuffer& gBuffer, class CameraComponent* camera);

	Image ambientOcclusionMap;

	static HBAOParameters gtaoParameters;
	static bool enable;
private:
	void CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
	void CreateAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportSize);
	void CreateRandomNoiseTexture(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);
	void CreateRenderPass(const GraphicsAPI& graphicsAPI, VkExtent2D viewportSize);
	void CreatePipeline(const GraphicsAPI& graphicsAPI, VkExtent2D viewportSize);

	AmbientOcclusionBlur ambientOcclusionBlur;

	VkFramebuffer framebuffer;

	Image noiseTexture;

	VkRenderPass renderPass;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	DescriptorSetManager< HorizonBasedAmbientOcclusion* > descriptorSetManager;

	VertexBuffer fullscreenQuadVertices;
	IndexBuffer fullscreenQuadIndices;

	VkExtent2D viewportSize;
	VkExtent2D noiseTextureSize;

	UniformBuffer<HBAOParameters> gtaoParametersBuffer;
};


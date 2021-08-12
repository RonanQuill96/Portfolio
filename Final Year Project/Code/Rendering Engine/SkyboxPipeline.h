#pragma once

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "LogicalDevice.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"

#include "ImageUtils.h"

#include "DescriptorSetManager.h"

#include <array>

struct PerSkyBoxBufferInfo
{
    glm::mat4 viewProjection;
};

struct SkyBoxVertex
{
    SkyBoxVertex() = default;

    SkyBoxVertex(glm::vec3 pPos)
        : pos(pPos) {}

    glm::vec3 pos;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(SkyBoxVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 1> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(SkyBoxVertex, pos);

        return attributeDescriptions;
    }
};

struct SkyBox
{
	IndexBuffer indexBuffer;
	VertexBuffer vertexBuffer;

    Image image;
	VkSampler textureSampler;

	void Load(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapLoadInfo& skyBoxLoadInfo);
};

class SkyboxPipeline
{
public:
	void Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent);
	void CreatePipeline(const LogicalDevice& logicalDevice, VkRenderPass renderPass, VkExtent2D viewportExtent);
	void CleanUpRenderPassDependentObjects(VkDevice device);
	void CleanUp(VkDevice device);

    void RenderSkyBox(VkCommandBuffer commandBuffer, VkDevice device, SkyBox* skyBox, class CameraComponent* cameraComponent);

private:
    VkDescriptorSet CreateDescriptorSet(SkyBox* skyBox);

    void CreateDescriptorSetLayout(const LogicalDevice& logicalDevice);

    UniformBuffer<PerSkyBoxBufferInfo> uniformBuffer;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

    DescriptorSetManager< SkyBox* > skyBoxDescriptorSetManager;
};


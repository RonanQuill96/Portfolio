#pragma once

#include "VulkanIncludes.h"

#include <array>

struct DebugLineVertex
{
    DebugLineVertex() = default;

    DebugLineVertex(glm::vec3 pPos, glm::vec3 pColour)
        : pos(pPos), colour(pColour) {}

    glm::vec3 pos;
    glm::vec3 colour;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(DebugLineVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(DebugLineVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(DebugLineVertex, colour);

        return attributeDescriptions;
    }
};

#include "DescriptorSetManager.h"

#include "GraphicsAPI.h"
#include "UniformBuffer.h"
#include "SwapChain.h"

#include "VertexBuffer.h"

struct DebugLine
{
    std::vector<DebugLineVertex> vertices;
    VertexBuffer vertexBuffer;
};

class DebugLinesPipeline
{
public:
    void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkRenderPass renderPass, VkExtent2D viewportExtent);

    void AddLine(std::vector<DebugLineVertex> vertices);

    void Render(VkCommandBuffer commandBuffer, glm::mat4 transform);
private:
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<std::shared_ptr<DebugLine>> lines;

    const GraphicsAPI* m_graphicsAPI;
    CommandPool* m_commandPool;
};


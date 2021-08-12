#pragma once

#include "VulkanIncludes.h"

#include "SkyboxPipeline.h"

#include "LogicalDevice.h"
#include "CommandPool.h"

class Scene;

class ForwardRenderer
{
public:
    void Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent, VkRenderPass renderPass);
    void CleanUp(VkDevice device);

    VkCommandBuffer PerformRenderPass(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo);

private:
    SkyboxPipeline skyBoxPipeline;
};


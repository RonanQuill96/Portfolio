#include "ForwardRenderer.h"

#include "CameraComponent.h"
#include "Scene.h"


#include "GlobalOptions.h"

void ForwardRenderer::Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent, VkRenderPass renderPass)
{
    skyBoxPipeline.Create(graphicsAPI, renderPass, viewportExtent);
}

void ForwardRenderer::CleanUp(VkDevice device)
{
    skyBoxPipeline.CleanUp(device);
}

VkCommandBuffer ForwardRenderer::PerformRenderPass(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo)
{
    std::vector<VkCommandBuffer> secondaryCommandBuffers;

    //CullLights skybox last to prevent overdraws
    SkyBox* skyBox = scene.GetSkyBox();
    if (skyBox != nullptr)
    {
        VkCommandBuffer skyBoxBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &commandBufferInheritanceInfo);
        secondaryCommandBuffers.push_back(skyBoxBuffer);

        VulkanDebug::BeginMarkerRegion(skyBoxBuffer, "Skybox Pass", glm::vec4(0.6f, 0.5f, 0.2f, 1.0f));
        skyBoxPipeline.RenderSkyBox(skyBoxBuffer, logicalDevice.GetVkDevice(), skyBox, scene.GetActiveCamera());
        VulkanDebug::EndMarkerRegion(skyBoxBuffer);
    }

    vkCmdExecuteCommands(commandBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());

    return commandBuffer;
}

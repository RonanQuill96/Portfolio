#include "LightingPass.h"

#include "Scene.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "VulkanDebug.h"

#include "DescriptorSetLayoutBuilder.h"

#include "GlobalOptions.h"

#include <array>

#include "ClusteredLightCulling.h"

void LightingPass::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain, VkRenderPass renderPass, const GBuffer& gBuffer)
{
    CreateFullscreenQuad(graphicsAPI, commandPool);

    perLightingPassInfoBuffer.Create(graphicsAPI);

    CreateDescriptors(graphicsAPI, gBuffer);

    directionalLightPipeline.Create(graphicsAPI, renderPass, swapChain.GetExtent(), perLightingPassDescriptorSetLayout);
    environmentLightPipeline.Create(graphicsAPI, renderPass, swapChain.GetExtent(), perLightingPassDescriptorSetLayout);

    clusteredLightCulling.Create(graphicsAPI, swapChain.GetExtent());
    clusteredLightCulling.CreatePointLightShader(graphicsAPI, renderPass, swapChain.GetExtent(), perLightingPassDescriptorSetLayout);
}

void LightingPass::CleanUp(VkDevice device)
{
    perLightingPassDescriptorSetManager.CleanUp();

    environmentLightPipeline.CleanUp(device);
    directionalLightPipeline.CleanUp(device);

    perLightingPassInfoBuffer.Cleanup();

    indexBuffer.Cleanup();
    vertexBuffer.Cleanup();
}

void LightingPass::Render(VkCommandBuffer commandBuffer, const LogicalDevice& logicalDevice, CommandPool& commandPool, const Scene& scene, VkCommandBufferInheritanceInfo commandBufferInheritanceInfo, VkExtent2D viewportExtent, Image& aoMap, VkSampler pointSampler)
{
    PerLightingPassInfo plpi;
    plpi.camPosition = scene.GetActiveCamera()->GetOwner().GetWorldPosition();
    plpi.inverseView = glm::inverse(scene.GetActiveCamera()->GetView());
    plpi.inverseProjection = glm::inverse(scene.GetActiveCamera()->GetProjection());
    perLightingPassInfoBuffer.Update(plpi);

    std::vector<VkCommandBuffer> secondaryCommandBuffers;

    VkCommandBuffer secondaryCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &commandBufferInheritanceInfo);
    secondaryCommandBuffers.push_back(secondaryCommandBuffer);

    // ----------------------------------------------------------------------------------------
    // Point Lights
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(secondaryCommandBuffer, "Point Lights", glm::vec4(0.6f, 0.5f, 0.2f, 1.0f));
    clusteredLightCulling.Render(secondaryCommandBuffer, vertexBuffer, indexBuffer, perLightingPassDescriptorSet, scene.GetActiveCamera(), viewportExtent);
    VulkanDebug::EndMarkerRegion(secondaryCommandBuffer);

    // ----------------------------------------------------------------------------------------
    // Directional Lights
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(secondaryCommandBuffer, "Directional Lights", glm::vec4(0.6f, 0.0f, 0.2f, 1.0f));
    const auto& directionalLights = scene.renderingScene.GetDirectionalLights();
    for (const DirectionalLightComponent* directionalLight : directionalLights)
    {
        directionalLightPipeline.Render(secondaryCommandBuffer, vertexBuffer, indexBuffer, directionalLight, perLightingPassDescriptorSet);
    }
    VulkanDebug::EndMarkerRegion(secondaryCommandBuffer);

    // ----------------------------------------------------------------------------------------
    // Environment Light
    // ----------------------------------------------------------------------------------------
    if (GlobalOptions::GetInstace().environmentLight)
    {
        VulkanDebug::BeginMarkerRegion(secondaryCommandBuffer, "Environment Lights", glm::vec4(0.6f, 0.0f, 1.0f, 1.0f));
        environmentLightPipeline.Render(secondaryCommandBuffer, vertexBuffer, indexBuffer, scene.GetEnvironmentMap(), perLightingPassDescriptorSet, aoMap, pointSampler);
        VulkanDebug::EndMarkerRegion(secondaryCommandBuffer);
    }

    if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    vkCmdExecuteCommands(commandBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());
}

void LightingPass::PerformLightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D viewportExtent)
{
    PerLightingPassInfo plpi;
    plpi.camPosition = scene.GetActiveCamera()->GetOwner().GetWorldPosition();
    plpi.inverseView = glm::inverse(scene.GetActiveCamera()->GetView());
    plpi.inverseProjection = glm::inverse(scene.GetActiveCamera()->GetProjection());
    perLightingPassInfoBuffer.Update(plpi);

    const std::vector<PointLightComponent*>& pointLights = scene.renderingScene.GetPointLights(); 
    clusteredLightCulling.CullLights(commandBuffer, scene.GetActiveCamera(), &pointLights, viewportExtent); 
}

void LightingPass::CreateDescriptors(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer)
{
    {
        DescriptorSetLayoutBuilder builder;
        builder
            .AddBindPoint(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBindPoint(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBindPoint(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBindPoint(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBindPoint(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(graphicsAPI.GetLogicalDevice().GetVkDevice(), &perLightingPassDescriptorSetLayout);
     
        perLightingPassDescriptorSetManager.Create(graphicsAPI.GetLogicalDevice(), perLightingPassDescriptorSetLayout, builder.GetLayoutBindings());
    }

    {
        perLightingPassDescriptorSet = perLightingPassDescriptorSetManager.AllocateDescriptorSet(this);

        VkSampler gBufferSampler = gBuffer.GetSampler();

        VkDescriptorBufferInfo perObjectBufferInfo = perLightingPassInfoBuffer.GetDescriptorInfo();
        VkDescriptorImageInfo normalsLayer = gBuffer.normals.image.GetImageInfo(gBufferSampler);
        VkDescriptorImageInfo albedoLayer = gBuffer.albedo.image.GetImageInfo(gBufferSampler);
        VkDescriptorImageInfo metalicRoughnessLayer = gBuffer.metalicRoughness.image.GetImageInfo(gBufferSampler);
        VkDescriptorImageInfo depthLayer = gBuffer.depth.image.GetImageInfo(gBufferSampler);

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
            .BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalsLayer)
            .BindImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &albedoLayer)
            .BindImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metalicRoughnessLayer)
            .BindImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthLayer)
            .Build(perLightingPassDescriptorSet, perLightingPassDescriptorSetManager.GetDeviceHandle());
    }
}

void LightingPass::CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    const std::vector <ScreenVertex> vertices =
    {
        ScreenVertex({-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}),
        ScreenVertex({-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
        ScreenVertex({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
        ScreenVertex({1.0f, -1.0f, 1.0f}, {1.0f, 1.0f})
    };

    vertexBuffer.Create(graphicsAPI, commandPool, vertices);

    const std::vector<uint32_t>  indices =
    {
        0, 1, 2,
        0, 2, 3,
    };

    indexBuffer.Create(graphicsAPI, commandPool, indices);
}


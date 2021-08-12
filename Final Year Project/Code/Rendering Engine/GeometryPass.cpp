#include "GeometryPass.h"

#include "CameraComponent.h"
#include "GameObject.h"

#include "GlobalOptions.h"

#include <array>

void GeometryPass::Create(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer)
{
    CreateRenderPass(graphicsAPI, gBuffer);
    CreateFrameBuffer(graphicsAPI, gBuffer);

    staticMeshPipeline.Create(graphicsAPI, renderPass, gBuffer.extent);
    staticMeshPBRTexturePipeline.Create(graphicsAPI, renderPass, gBuffer.extent);
}

void GeometryPass::CleanUp(VkDevice device)
{
    staticMeshPBRTexturePipeline.CleanUp(device);
    staticMeshPipeline.CleanUp(device);
    vkDestroyFramebuffer(device, gBufferFrameBuffer, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void GeometryPass::Render(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const GBuffer& gBuffer, const Scene& scene)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = gBufferFrameBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = gBuffer.extent;

    std::array<VkClearValue, 4> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[3].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.framebuffer = gBufferFrameBuffer;

    VkDevice device = logicalDevice.GetVkDevice();

    std::vector<VkCommandBuffer> secondaryCommandBuffers;

    CameraComponent* camera = scene.GetActiveCamera();
    const auto meshCompoents = scene.renderingScene.GetSceneMeshes(camera->GetViewFrustum());

    VkCommandBuffer secondaryCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    secondaryCommandBuffers.push_back(secondaryCommandBuffer);

    for (const MeshComponent* meshComponent : meshCompoents)
    {
        const Mesh& mesh = meshComponent->GetMesh();

        for (const auto& meshMaterial : mesh.materials)
        {
            if (meshMaterial.material->materialRenderMode == MaterialRenderMode::Opaque)
            {
                if (meshMaterial.material->materialType == MaterialType::PBRParameters)
                {
                    staticMeshPipeline.Render(secondaryCommandBuffer, meshComponent, meshMaterial);
                }
                else if (meshMaterial.material->materialType == MaterialType::PBRTextures)
                {
                    staticMeshPBRTexturePipeline.Render(secondaryCommandBuffer, meshComponent, meshMaterial);
                }
                else
                {
                    throw std::runtime_error("Unsupported material type");
                }
            }               
        }
    }
   

    if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    vkCmdExecuteCommands(commandBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());

    vkCmdEndRenderPass(commandBuffer);
}

void GeometryPass::CreateRenderPass(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer)
{
    VkAttachmentDescription common{};
    common.samples = VK_SAMPLE_COUNT_1_BIT;
    common.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    common.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    common.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    common.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    common.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkAttachmentDescription normals = common;
    normals.format = gBuffer.normals.format;
    normals.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription albedo = common;
    albedo.format = gBuffer.albedo.format;
    albedo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription metalicRoughness = common;
    metalicRoughness.format = gBuffer.metalicRoughness.format;
    metalicRoughness.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depth = common;
    depth.format = gBuffer.depth.format;
    depth.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::vector<VkAttachmentReference> colorReferences =
    {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 3;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 4> attachments =
    {
        normals,
        albedo,
        metalicRoughness,
        depth
    };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void GeometryPass::CreateFrameBuffer(const GraphicsAPI& graphicsAPI, const GBuffer& gBuffer)
{ 
    std::array<VkImageView, 4> attachments =
    {
        gBuffer.normals.image.textureImageView,
        gBuffer.albedo.image.textureImageView,
        gBuffer.metalicRoughness.image.textureImageView,
        gBuffer.depth.image.textureImageView
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.width = gBuffer.extent.width;
    framebufferInfo.height = gBuffer.extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(graphicsAPI.GetLogicalDevice().GetVkDevice(), &framebufferInfo, nullptr, &gBufferFrameBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

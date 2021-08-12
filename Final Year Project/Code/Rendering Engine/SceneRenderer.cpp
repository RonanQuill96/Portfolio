#include "SceneRenderer.h"

#include "VulkanDebug.h"

#include "CameraComponent.h"

#include <array>

void SceneRenderer::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain)
{
    CreateRenderImages(graphicsAPI, commandPool, swapChain);

    CreateDepthBuffers(graphicsAPI, commandPool, swapChain);

    CreateRenderPass(graphicsAPI.GetLogicalDevice(), graphicsAPI.GetPhysicalDevice(), swapChain);

    CreateFrameBuffers(graphicsAPI.GetLogicalDevice(), swapChain);
 
    forwardRenderer.Create(graphicsAPI, swapChain.GetExtent(), sceneRenderPass);
    deferredRenderer.Create(graphicsAPI, commandPool, swapChain, sceneRenderPass);

    uiRenderer.Create(graphicsAPI, swapChain.GetExtent(), commandPool, sceneRenderPass);
}

void SceneRenderer::OnSwapChainChanged(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    vkDestroyRenderPass(device, sceneRenderPass, nullptr);

    for (size_t i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < depthImageViews.size(); i++)
    {
        vkDestroyImageView(device, depthImageViews[i], nullptr);
        vkDestroyImage(device, depthImages[i], nullptr);
        vkFreeMemory(device, depthImagesMemory[i], nullptr);
    }

    //Recreate depth buffer
    CreateDepthBuffers(graphicsAPI, commandPool, swapChain);

    //Recreate framebuffers
    CreateFrameBuffers(graphicsAPI.GetLogicalDevice(), swapChain);

    //Recreate renderpass
    CreateRenderPass(graphicsAPI.GetLogicalDevice(), graphicsAPI.GetPhysicalDevice(), swapChain);

    //Recreate pipelines
}

void SceneRenderer::Prepare(const Scene& scene)
{
    CameraComponent* camera = scene.GetActiveCamera();
    const auto& meshCompoents = scene.renderingScene.GetMeshComponents();

    for (const MeshComponent* meshComponent : meshCompoents)
    {
        PerObjectBufferInfo perObjectInfo{};
        perObjectInfo.wvm = camera->GetProjection() * camera->GetView() * meshComponent->GetOwner().GetWorldMatrix();
        perObjectInfo.worldMatrix = meshComponent->GetOwner().GetWorldMatrix();
        perObjectInfo.worldMatrixInvTrans = glm::transpose(glm::inverse(perObjectInfo.worldMatrix));
        perObjectInfo.view = camera->GetView();

        const Mesh& mesh = meshComponent->GetMesh();
        meshComponent->perObjectBuffer.Update(perObjectInfo);
    }
}

void SceneRenderer::CleanUp(VkDevice device)
{
    forwardRenderer.CleanUp(device);
    deferredRenderer.CleanUp(device);
    uiRenderer.CleanUp();

    vkDestroyRenderPass(device, sceneRenderPass, nullptr);

    for (size_t i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < depthImageViews.size(); i++)
    {
        vkDestroyImageView(device, depthImageViews[i], nullptr);
        vkDestroyImage(device, depthImages[i], nullptr);
        vkFreeMemory(device, depthImagesMemory[i], nullptr);
    }
}

void SceneRenderer::UpdateUI(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{    
    //uiRenderer.NewFrame();
    uiRenderer.UpdateBuffers(graphicsAPI, commandPool);
}

void SceneRenderer::GenerateGBuffer(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, VkExtent2D swapChainExtent, size_t imageIndex)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    // ----------------------------------------------------------------------------------------
	// First Step: CullLights G-Buffer 
	// ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "GBuffer Generation Pass", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    deferredRenderer.RenderGBuffer(commandBuffer, logicalDevice, commandPool, scene);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    // ----------------------------------------------------------------------------------------
    // Copy over GBuffer depth values
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Copy GBuffer Depth Buffer", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    VkImageSubresourceRange fbsubresourceRange{};
    fbsubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    fbsubresourceRange.baseMipLevel = 0;
    fbsubresourceRange.levelCount = 1;
    fbsubresourceRange.layerCount = 1;
    graphicsAPI.TransitionImageLayout(commandBuffer, depthImages[imageIndex], VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, fbsubresourceRange);

    VkImageSubresourceRange imsubresourceRange{};
    imsubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imsubresourceRange.baseMipLevel = 0;
    imsubresourceRange.levelCount = 1;
    imsubresourceRange.layerCount = 1;
    imsubresourceRange.baseArrayLayer = 0;
    graphicsAPI.TransitionImageLayout(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imsubresourceRange);

    VkImageBlit imageBlit;
    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageBlit.srcSubresource.baseArrayLayer = 0;
    imageBlit.srcSubresource.layerCount = 1;
    imageBlit.srcSubresource.mipLevel = 0;
    imageBlit.srcOffsets[0] = { 0, 0, 0 };
    imageBlit.srcOffsets[1] = { static_cast<int32_t>(swapChainExtent.width), static_cast<int32_t>(swapChainExtent.height), 1 };

    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageBlit.dstSubresource.baseArrayLayer = 0;
    imageBlit.dstSubresource.layerCount = 1;
    imageBlit.dstSubresource.mipLevel = 0;
    imageBlit.dstOffsets[0] = { 0, 0, 0 };
    imageBlit.dstOffsets[1] = { static_cast<int32_t>(swapChainExtent.width), static_cast<int32_t>(swapChainExtent.height), 1 };

    vkCmdBlitImage(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);

    graphicsAPI.TransitionImageLayout(commandBuffer, depthImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fbsubresourceRange);
    graphicsAPI.TransitionImageLayout(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imsubresourceRange);

    VulkanDebug::EndMarkerRegion(commandBuffer);
}

void SceneRenderer::LightingCull(VkCommandBuffer commandBuffer, const Scene& scene, VkExtent2D swapChainExtent)
{
    deferredRenderer.PerformLightingCull(commandBuffer, scene, swapChainExtent);
}

void SceneRenderer::PerformShading(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, VkExtent2D swapChainExtent, size_t imageIndex)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    // ----------------------------------------------------------------------------------------
    // Clear swap chain image and start render pass
    // ----------------------------------------------------------------------------------------
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;// static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = sceneRenderPass;
    inheritanceInfo.framebuffer = framebuffers[imageIndex];

    // ----------------------------------------------------------------------------------------
    // Perform Lighting pass on gbuffer
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Lighting Pass", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    deferredRenderer.PerformLightingPass(commandBuffer, logicalDevice, commandPool, scene, inheritanceInfo, swapChainExtent);
    VulkanDebug::EndMarkerRegion(commandBuffer);


    // ----------------------------------------------------------------------------------------
    // Normal forward rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Forward Rendering Pass", glm::vec4(0.0f, 0.5f, 0.2f, 1.0f));
    forwardRenderer.PerformRenderPass(commandBuffer, logicalDevice, commandPool, scene, inheritanceInfo);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    // ----------------------------------------------------------------------------------------
    // UI rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Forward Rendering Pass", glm::vec4(0.6f, 0.2f, 0.0f, 1.0f));
    VkCommandBuffer uiCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    uiRenderer.DrawCurrentFrame(uiCommandBuffer);
    if (vkEndCommandBuffer(uiCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
    vkCmdExecuteCommands(commandBuffer, 1, &uiCommandBuffer);
    VulkanDebug::EndMarkerRegion(commandBuffer);


    vkCmdEndRenderPass(commandBuffer);
}

void SceneRenderer::CreateDepthBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain)
{
    size_t swapChainImageCount = swapChain.GetImageCount();
    VkExtent2D swapChainExtent = swapChain.GetExtent();

    depthImages.resize(swapChainImageCount);
    depthImagesMemory.resize(swapChainImageCount);
    depthImageViews.resize(swapChainImageCount);

    for (size_t i = 0; i < swapChainImageCount; i++)
    {
        VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat();
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &depthImages[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, depthImages[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImagesMemory[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, depthImages[i], depthImagesMemory[i], 0);

        depthImageViews[i] = ImageUtils::CreateImageView(graphicsAPI.GetLogicalDevice(), depthImages[i], graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);

        graphicsAPI.TransitionImageLayoutImmediate(commandPool, depthImages[i], graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}

void SceneRenderer::CreateFrameBuffers(const LogicalDevice& logicalDevice, const SwapChain& swapChain)
{
    framebuffers.resize(swapChain.GetImageCount());

    VkExtent2D swapChainExtent = swapChain.GetExtent();
    for (size_t i = 0; i < framebuffers.size(); i++)
    {
        std::array<VkImageView, 2> attachments =
        {
            renderImages[i].textureImageView,
            depthImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = sceneRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice.GetVkDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void SceneRenderer::CreateRenderPass(const LogicalDevice& logicalDevice, const PhysicalDevice& physicalDevice, const SwapChain& swapChain)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = physicalDevice.GetOptimalDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; //VK_ATTACHMENT_LOAD_OP_CLEAR
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;// VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments =
    {
        colorAttachment,
        depthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkDevice device = logicalDevice.GetVkDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &sceneRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

}

void SceneRenderer::CreateSyncObjects()
{
}

void SceneRenderer::CreateRenderImages(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain)
{
    renderImages.resize(swapChain.GetImageCount());

    for (Image& image : renderImages)
    {
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChain.GetExtent().width;
        imageInfo.extent.height = swapChain.GetExtent().height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = swapChain.GetImageFormat();
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image.textureImage) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image.textureImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &image.textureImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image.textureImage, image.textureImageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image.textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChain.GetImageFormat();
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &image.textureImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
   
}

void SceneRenderer::RenderScene(VkCommandBuffer commandBuffer, const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, VkExtent2D swapChainExtent, size_t imageIndex)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

	// ----------------------------------------------------------------------------------------
	// First Step: CullLights G-Buffer 
	// ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "GBuffer Generation Pass", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	deferredRenderer.RenderGBuffer(commandBuffer, logicalDevice, commandPool, scene);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    // ----------------------------------------------------------------------------------------
    // Copy over GBuffer depth values
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Copy GBuffer Depth Buffer", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    VkImageSubresourceRange fbsubresourceRange{};
    fbsubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    fbsubresourceRange.baseMipLevel = 0;
    fbsubresourceRange.levelCount = 1;
    fbsubresourceRange.layerCount = 1;
    graphicsAPI.TransitionImageLayout(commandBuffer, depthImages[imageIndex], VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, fbsubresourceRange);

    VkImageSubresourceRange imsubresourceRange{};
    imsubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imsubresourceRange.baseMipLevel = 0;
    imsubresourceRange.levelCount = 1;
    imsubresourceRange.layerCount = 1;
    imsubresourceRange.baseArrayLayer = 0;
    graphicsAPI.TransitionImageLayout(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imsubresourceRange);

    VkImageBlit imageBlit;
    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageBlit.srcSubresource.baseArrayLayer = 0;
    imageBlit.srcSubresource.layerCount = 1;
    imageBlit.srcSubresource.mipLevel = 0;
    imageBlit.srcOffsets[0] = { 0, 0, 0 };
    imageBlit.srcOffsets[1] = { static_cast<int32_t>(swapChainExtent.width), static_cast<int32_t>(swapChainExtent.height), 1 };

    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageBlit.dstSubresource.baseArrayLayer = 0;
    imageBlit.dstSubresource.layerCount = 1;
    imageBlit.dstSubresource.mipLevel = 0;
    imageBlit.dstOffsets[0] = { 0, 0, 0 };
    imageBlit.dstOffsets[1] = { static_cast<int32_t>(swapChainExtent.width), static_cast<int32_t>(swapChainExtent.height), 1 };

    vkCmdBlitImage(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, depthImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);

    graphicsAPI.TransitionImageLayout(commandBuffer, depthImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fbsubresourceRange);
    graphicsAPI.TransitionImageLayout(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imsubresourceRange);

    VulkanDebug::EndMarkerRegion(commandBuffer);

	// ----------------------------------------------------------------------------------------
	// Clear swap chain image and start render pass
	// ----------------------------------------------------------------------------------------
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sceneRenderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;// static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = sceneRenderPass;
    inheritanceInfo.framebuffer = framebuffers[imageIndex];

    // ----------------------------------------------------------------------------------------
    // Perform Lighting pass on gbuffer
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Lighting Pass", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    deferredRenderer.PerformLightingPass(commandBuffer, logicalDevice, commandPool, scene, inheritanceInfo, swapChainExtent);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    
    // ----------------------------------------------------------------------------------------
    // Normal forward rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Forward Rendering Pass", glm::vec4(0.0f, 0.5f, 0.2f, 1.0f));
    forwardRenderer.PerformRenderPass(commandBuffer, logicalDevice, commandPool, scene, inheritanceInfo);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    // ----------------------------------------------------------------------------------------
    // UI rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(commandBuffer, "Forward Rendering Pass", glm::vec4(0.6f, 0.2f, 0.0f, 1.0f));
    VkCommandBuffer uiCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    uiRenderer.DrawCurrentFrame(uiCommandBuffer);    
    if (vkEndCommandBuffer(uiCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
    vkCmdExecuteCommands(commandBuffer, 1, &uiCommandBuffer);
    VulkanDebug::EndMarkerRegion(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

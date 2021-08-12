#include "ClusteredDeferredShading.h"

#include "GlobalOptions.h"

DebugLinesPipeline ClusteredDeferredShading::debugLinesPipeline;

void ClusteredDeferredShading::Initialise(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool)
{
    swapChainExtent = swapChain.GetExtent();

    CreateRenderImages(graphicsAPI, commandPool, swapChain);
    CreateOpaqueRenderPass(graphicsAPI, swapChain, commandPool);
    CreateSwapChainRenderPass(graphicsAPI, swapChain);

    CreateSyncObjects(graphicsAPI);

    deferredRenderer.Create(graphicsAPI, commandPool, swapChain, opaqueRenderPass);
    mboit.Create(graphicsAPI, swapChain, commandPool, depthImageViews,  swapChainRenderPass);
    forwardRenderer.Create(graphicsAPI, swapChain.GetExtent(), opaqueRenderPass);
    uiRenderer.Create(graphicsAPI, swapChain.GetExtent(), commandPool, swapChainRenderPass);
    groundTruthAmbientOcclusion.Create(graphicsAPI, commandPool, swapChain.GetExtent());

    debugLinesPipeline.Create(graphicsAPI, commandPool, opaqueRenderPass, swapChainExtent);
}

void ClusteredDeferredShading::Prepare(const GraphicsAPI& graphicsAPI, CommandPool& generalCommandPool, const Scene& scene)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    // ----------------------------------------------------------------------------------------
    // Preparation
    // ----------------------------------------------------------------------------------------
    CameraComponent* camera = scene.GetActiveCamera();
    const auto meshCompoents = scene.renderingScene.GetSceneMeshes(camera->GetViewFrustum());

    for (const MeshComponent* meshComponent : meshCompoents)
    {
        PerObjectBufferInfo perObjectInfo{};
        perObjectInfo.wvm = camera->GetProjection() * camera->GetView() * meshComponent->GetOwner().GetWorldMatrix();
        perObjectInfo.worldMatrix = meshComponent->GetOwner().GetWorldMatrix();
        perObjectInfo.worldMatrixInvTrans = glm::inverse(perObjectInfo.worldMatrix);// glm::transpose(glm::inverse(perObjectInfo.worldMatrix));
        perObjectInfo.view = camera->GetView();

        const Mesh& mesh = meshComponent->GetMesh();
        meshComponent->perObjectBuffer.Update(perObjectInfo);
    }

    uiRenderer.UpdateBuffers(graphicsAPI, generalCommandPool);

    mboit.Prepare(scene);
}

void ClusteredDeferredShading::RenderFrame(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore, VkFence fence, SwapChain& swapChain)
{
    // ----------------------------------------------------------------------------------------
    // Generate GBuffer
    // ----------------------------------------------------------------------------------------
    GenerateGBuffer(graphicsAPI, commandPool, scene, imageIndex, currentFrame, imageAvailableSemaphore);

    // ----------------------------------------------------------------------------------------
    // Perform Lighting Culling
    // ----------------------------------------------------------------------------------------
    GenerateClusters(graphicsAPI, commandPool, scene, imageIndex, currentFrame, imageAvailableSemaphore);

    // ----------------------------------------------------------------------------------------
    // Transparency Absorbance Pass
    // ----------------------------------------------------------------------------------------
    RenderTransparencyAbsorbance(graphicsAPI, commandPool, scene, imageIndex, currentFrame);

    // ----------------------------------------------------------------------------------------
    // Ambient Occlusion Map Pass
    // ----------------------------------------------------------------------------------------
    RenderAmbientOcclusionMap(graphicsAPI, commandPool, scene, imageIndex, currentFrame);

    // ----------------------------------------------------------------------------------------
    // Perform Lighting Culling
    // ----------------------------------------------------------------------------------------
    CullLights(graphicsAPI, commandPool, scene, imageIndex, currentFrame);

    // ----------------------------------------------------------------------------------------
    // Transparency Transmittance
    // ----------------------------------------------------------------------------------------
    RenderTransparencyTransmittance(graphicsAPI, commandPool, scene, imageIndex, currentFrame);

    // ----------------------------------------------------------------------------------------
    // Shading image
    // ----------------------------------------------------------------------------------------
    OpaqueLightingPass(graphicsAPI, commandPool, scene, imageIndex, currentFrame);

    // ----------------------------------------------------------------------------------------
    // Composite Transparent & Opauqe meshes
    // ----------------------------------------------------------------------------------------
    CompisiteTransparency(graphicsAPI, commandPool, scene, imageIndex, currentFrame, renderFinishedSemaphore, fence);   
}

void ClusteredDeferredShading::GenerateGBuffer(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer commandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    // ----------------------------------------------------------------------------------------
    // Render To G-Buffer 
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

    // Copy region for transfer from framebuffer to cube face
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent.width = swapChainExtent.width;
    copyRegion.extent.height = swapChainExtent.height;
    copyRegion.extent.depth = 1;

    // Put image copy into command buffer
    vkCmdCopyImage(
        commandBuffer,
        deferredRenderer.gBuffer.depth.image.textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        depthImages[imageIndex],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion);

    graphicsAPI.TransitionImageLayout(commandBuffer, depthImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fbsubresourceRange);
    graphicsAPI.TransitionImageLayout(commandBuffer, deferredRenderer.gBuffer.depth.image.textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imsubresourceRange);

    VulkanDebug::EndMarkerRegion(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(commandBuffer);
    queueSubmitInfo.AddWait(imageAvailableSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].gBufferGenerationSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue());
}

void ClusteredDeferredShading::GenerateClusters(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer lighCullingCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    VulkanDebug::BeginMarkerRegion(lighCullingCommandBuffer, "Cluster Generation Pass", glm::vec4(0.5f, 0.2f, 0.7f, 1.0f));
    deferredRenderer.lightingPass.clusteredLightCulling.CreateClusters(lighCullingCommandBuffer, scene.GetActiveCamera(), swapChainExtent);
     VulkanDebug::EndMarkerRegion(lighCullingCommandBuffer);
    if (vkEndCommandBuffer(lighCullingCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(lighCullingCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].gBufferGenerationSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].createClustersSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetComputeQueue());
}

void ClusteredDeferredShading::CullLights(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer lighCullingCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    VulkanDebug::BeginMarkerRegion(lighCullingCommandBuffer, "Light Culling Pass", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    deferredRenderer.lightingPass.clusteredLightCulling.MarkActiveClusters(lighCullingCommandBuffer, scene.GetActiveCamera(), deferredRenderer.gBuffer, swapChainExtent);
    deferredRenderer.PerformLightingCull(lighCullingCommandBuffer, scene, swapChainExtent);
    VulkanDebug::EndMarkerRegion(lighCullingCommandBuffer);
    if (vkEndCommandBuffer(lighCullingCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(lighCullingCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].gBufferGenerationSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].createClustersSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].absorbanceSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].lightCulllingSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetComputeQueue());
}

void ClusteredDeferredShading::RenderAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer ambientOcclusionMapCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    VulkanDebug::BeginMarkerRegion(ambientOcclusionMapCommandBuffer, "Ambient Occlusion Map Pass", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
   
    groundTruthAmbientOcclusion.RenderAmbientOcclusionMap(graphicsAPI, commandPool, ambientOcclusionMapCommandBuffer, deferredRenderer.gBuffer, scene.GetActiveCamera());

    VulkanDebug::EndMarkerRegion(ambientOcclusionMapCommandBuffer);
    if (vkEndCommandBuffer(ambientOcclusionMapCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(ambientOcclusionMapCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].gBufferGenerationSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].ambientOcclusionGenerationSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue());
}

void ClusteredDeferredShading::RenderTransparencyAbsorbance(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer absorbancePassCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);
    VulkanDebug::BeginMarkerRegion(absorbancePassCommandBuffer, "Transparency Absorance Pass", glm::vec4(0.7, 1.0f, 0.2, 1.0f));
    mboit.RenderSceneAbsorbance(logicalDevice, commandPool, absorbancePassCommandBuffer, scene, imageIndex);
    VulkanDebug::EndMarkerRegion(absorbancePassCommandBuffer);

    if (vkEndCommandBuffer(absorbancePassCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(absorbancePassCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].gBufferGenerationSemaphore, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].createClustersSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].absorbanceSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue());
}

void ClusteredDeferredShading::RenderTransparencyTransmittance(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer transmittancePassCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);
    VulkanDebug::BeginMarkerRegion(transmittancePassCommandBuffer, "Transparency Transmittance Pass", glm::vec4(0.65, 0.2, 1.0f, 1.0f));
    
    mboit.RenderSceneTransmittanceClustered(logicalDevice, commandPool, transmittancePassCommandBuffer, scene, imageIndex,
            deferredRenderer.lightingPass.clusteredLightCulling.pointLightInfoBuffer,
            deferredRenderer.lightingPass.clusteredLightCulling.culledLightsPerCluster,
            deferredRenderer.lightingPass.clusteredLightCulling.cluster_x_count,
            deferredRenderer.lightingPass.clusteredLightCulling.cluster_y_count,
            deferredRenderer.lightingPass.clusteredLightCulling.cluster_z_slices,
            deferredRenderer.lightingPass.clusteredLightCulling.cluster_size_x);
   
    VulkanDebug::EndMarkerRegion(transmittancePassCommandBuffer);

    if (vkEndCommandBuffer(transmittancePassCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(transmittancePassCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].gBufferGenerationSemaphore, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].absorbanceSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].lightCulllingSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].transmittanceSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue());
}

void ClusteredDeferredShading::OpaqueLightingPass(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer opaqueLightingCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    // ----------------------------------------------------------------------------------------
    // Clear swap chain image and start render pass
    // ----------------------------------------------------------------------------------------
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = opaqueRenderPass;
    renderPassInfo.framebuffer = opaqueFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;// static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(opaqueLightingCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = opaqueRenderPass;
    inheritanceInfo.framebuffer = opaqueFramebuffers[imageIndex];

    // ----------------------------------------------------------------------------------------
    // Perform Lighting pass on gbuffer
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(opaqueLightingCommandBuffer, "Lighting Pass", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    deferredRenderer.PerformLightingPass(opaqueLightingCommandBuffer, logicalDevice, commandPool, scene, inheritanceInfo, swapChainExtent, groundTruthAmbientOcclusion.ambientOcclusionMap);
    VulkanDebug::EndMarkerRegion(opaqueLightingCommandBuffer);

    // ----------------------------------------------------------------------------------------
    // Normal forward rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(opaqueLightingCommandBuffer, "Forward Rendering Pass", glm::vec4(0.0f, 0.5f, 0.2f, 1.0f));
    forwardRenderer.PerformRenderPass(opaqueLightingCommandBuffer, logicalDevice, commandPool, scene,  inheritanceInfo);
    VulkanDebug::EndMarkerRegion(opaqueLightingCommandBuffer);

    // ----------------------------------------------------------------------------------------
    // Debug Lines rendering
    // ----------------------------------------------------------------------------------------
    VulkanDebug::BeginMarkerRegion(opaqueLightingCommandBuffer, "Debug Lines rendering", glm::vec4(0.7f, 0.5f, 0.2f, 1.0f));
    VkCommandBuffer secondaryCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    debugLinesPipeline.Render(secondaryCommandBuffer, scene.GetActiveCamera()->GetProjection() * scene.GetActiveCamera()->GetView());
    if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
    vkCmdExecuteCommands(opaqueLightingCommandBuffer, 1, &secondaryCommandBuffer);
    VulkanDebug::EndMarkerRegion(opaqueLightingCommandBuffer);
    

    vkCmdEndRenderPass(opaqueLightingCommandBuffer);
    if (vkEndCommandBuffer(opaqueLightingCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(opaqueLightingCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].lightCulllingSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].ambientOcclusionGenerationSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    queueSubmitInfo.AddSignal(frameData[currentFrame].shadingSemaphore);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue());
}

void ClusteredDeferredShading::CompisiteTransparency(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore renderFinishedSemaphore, VkFence fence)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkCommandBuffer compositionCommandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    // ----------------------------------------------------------------------------------------
    // Clear swap chain image and start render pass
    // ----------------------------------------------------------------------------------------
    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChainRenderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(compositionCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VulkanDebug::BeginMarkerRegion(compositionCommandBuffer, "Transparency Composition Pass", glm::vec4(0.7, 0.2f, 0.2, 1.0f));
    mboit.CompositieScene(logicalDevice, commandPool, compositionCommandBuffer, opaqueShadingResults[imageIndex], imageIndex);
    VulkanDebug::EndMarkerRegion(compositionCommandBuffer);

    VulkanDebug::BeginMarkerRegion(compositionCommandBuffer, "Render UI", glm::vec4(0.6f, 0.2f, 0.0f, 1.0f));
    uiRenderer.DrawCurrentFrame(compositionCommandBuffer);
    VulkanDebug::EndMarkerRegion(compositionCommandBuffer);

    vkCmdEndRenderPass(compositionCommandBuffer);
    if (vkEndCommandBuffer(compositionCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    QueueSubmitInfo queueSubmitInfo;
    queueSubmitInfo.AddCommandBuffer(compositionCommandBuffer);
    queueSubmitInfo.AddWait(frameData[currentFrame].transmittanceSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    queueSubmitInfo.AddWait(frameData[currentFrame].shadingSemaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    queueSubmitInfo.AddSignal(renderFinishedSemaphore);

    logicalDevice.ResetFences(1, &fence);
    queueSubmitInfo.SubmitToQueue(logicalDevice.GetGraphicsQueue(), fence);
}


void ClusteredDeferredShading::CreateOpaqueRenderPass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

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
    depthAttachment.format = graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat();
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &opaqueRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

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

    opaqueFramebuffers.resize(swapChain.GetImageCount());

    for (size_t i = 0; i < opaqueFramebuffers.size(); i++)
    {
        std::array<VkImageView, 2> attachments =
        {
            opaqueShadingResults[i].textureImageView,
            depthImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = opaqueRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &opaqueFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void ClusteredDeferredShading::CreateSwapChainRenderPass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    std::vector<VkAttachmentReference> colorReferences =
    {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 1> attachments =
    {
        colorAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &swapChainRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }


    VkExtent2D swapChainExtent = swapChain.GetExtent();

    // Frame buffers
    swapChainFramebuffers.resize(swapChain.GetImageCount());

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
    {
        std::array<VkImageView, 1> attachments =
        {
            swapChain.GetImageView(i)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = swapChainRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void ClusteredDeferredShading::CreateRenderImages(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain)
{
    opaqueShadingResults.resize(swapChain.GetImageCount());

    for (Image& image : opaqueShadingResults)
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

void ClusteredDeferredShading::CreateSyncObjects(const GraphicsAPI& graphicsAPI)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].gBufferGenerationSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].createClustersSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].lightCulllingSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].ambientOcclusionGenerationSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].absorbanceSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].transmittanceSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].shadingSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].trasnparencyCompositionSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

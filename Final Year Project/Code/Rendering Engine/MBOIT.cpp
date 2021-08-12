#include "MBOIT.h"

#include <array>

#include "GraphicsPipelineBuilder.h"
#include "DescriptorSetLayoutBuilder.h"
#include "Vertex.h"

#include "GlobalOptions.h"

#include "EnvironmentMap.h"

#include "PointLightComponent.h"

float MBOIT::overestimation = 0.04f;
float MBOIT::bias = 5 * 1e-5;

void MBOIT::Create(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews, VkRenderPass compositeRenderPass)
{
    extent = swapChain.GetExtent();

    CreateAbsorancePass(graphicsAPI, swapChain, commandPool, depthImageViews);
    CreateTransmittancePass(graphicsAPI, swapChain, commandPool, depthImageViews);
    CreateCompositePass(graphicsAPI, swapChain, commandPool, compositeRenderPass);

    //sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;

    VkPhysicalDeviceProperties properties = graphicsAPI.GetPhysicalDevice().GetProperties();

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(graphicsAPI.GetLogicalDevice().GetVkDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }

    mboitStaticMeshPBRParametersPipeline.Create(graphicsAPI, swapChain, absoranceRenderPass, transmittanceRenderPass, trasnmittancePassDescriptorSetLayout);
    mboitStaticMeshPBRTexturePipeline.Create(graphicsAPI, swapChain, absoranceRenderPass, transmittanceRenderPass, trasnmittancePassDescriptorSetLayout);
}

#include "CameraComponent.h"
#include "GameObject.h"

void MBOIT::Prepare(const Scene& scene)
{
    transparentMeshBounds = {};

    CameraComponent* camera = scene.GetActiveCamera();
    const auto meshCompoents = scene.renderingScene.GetSceneMeshes(camera->GetViewFrustum());
    for (const MeshComponent* meshComponent : meshCompoents)
    {
        const Mesh& mesh = meshComponent->GetMesh();

        for (const auto& meshMaterial : mesh.materials)
        {
            //TODO check if material is transparent
            if (meshMaterial.material->materialRenderMode == MaterialRenderMode::Transparent)
            {
               transparentMeshBounds = AABB::GetMaxAABB(transparentMeshBounds, mesh.aabb);
               break;
            }
        }
    }

    //Transform to view space
    //transparentMeshBounds = transparentMeshBounds.Transform(camera->GetView());

    auto TransformPoint = [](const glm::mat4& mat, const glm::vec3& vec) -> glm::vec3
    {
        glm::vec4 transVec = mat * glm::vec4(vec.x, vec.y, vec.z, 1.0f);
        if (transVec.w != 1.0f)
            transVec /= transVec.w;
        return glm::vec3(transVec.x, transVec.y, transVec.z);
    };

    glm::vec3 transformedCenter = TransformPoint(camera->GetView(), transparentMeshBounds.GetCenter());

    // Add offset of 0.1 for e.g. point data sets where additonal vertices may be added in the shader for quads. ??
    float minViewZ = transformedCenter.z + transparentMeshBounds.GetRadius();// transparentMeshBounds.minVertex.z + 0.1;
    float maxViewZ = transformedCenter.z - transparentMeshBounds.GetRadius();// transparentMeshBounds.maxVertex.z - 0.1;
    minViewZ = std::max(-minViewZ, camera->GetNearPlane());
    maxViewZ = std::min(-maxViewZ, camera->GetFarPlane());
    minViewZ = std::min(minViewZ, camera->GetFarPlane());
    maxViewZ = std::max(maxViewZ, camera->GetNearPlane());

    lnDepthMin = std::log(minViewZ);
    lnDepthMax = std::log(maxViewZ);

    MBOITPerFrameInfo pfi{};
    pfi.cameraPosition = camera->GetOwner().GetWorldPosition();
    pfi.lnDepthMin = lnDepthMin;
    pfi.lnDepthMax = lnDepthMax;

    //TODO questions over thread safety of this
    perFrameInfoBuffer.Update(pfi);
}

#include "ClusteredLightCulling.h"

void MBOIT::RenderSceneAbsorbance(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const Scene& scene, size_t imageIndex)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = absoranceRenderPass;
    renderPassInfo.framebuffer = absoranceFrameBuffer[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = absoranceRenderPass;
    inheritanceInfo.framebuffer = absoranceFrameBuffer[imageIndex];

    VkDevice device = logicalDevice.GetVkDevice();

    //TODO: Having this many secondary command buffers is bad for performance
    //Cut it down to one seccondary command buffer per thread
    std::vector<VkCommandBuffer> secondaryCommandBuffers;

    CameraComponent* camera = scene.GetActiveCamera();
    const auto meshCompoents = scene.renderingScene.GetSceneMeshes(camera->GetViewFrustum());

    VkCommandBuffer secondaryCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    secondaryCommandBuffers.push_back(secondaryCommandBuffer);

    //Push constants
    MBOITPushConstants mboitPushConstants;
    mboitPushConstants.lnDepthMin = lnDepthMin;
    mboitPushConstants.lnDepthMax = lnDepthMax;

    mboitPushConstants.clusterCounts = { ClusteredLightCulling::cluster_x_count, ClusteredLightCulling::cluster_y_count, ClusteredLightCulling::cluster_z_slices };
    mboitPushConstants.clusterSizeX = ClusteredLightCulling::cluster_size_x;
    mboitPushConstants.zNear = camera->GetNearPlane();
    mboitPushConstants.zFar = camera->GetFarPlane();
    mboitPushConstants.scale = (float)ClusteredLightCulling::cluster_z_slices / std::log2f(mboitPushConstants.zFar / mboitPushConstants.zNear);
    mboitPushConstants.bias = -((float)ClusteredLightCulling::cluster_z_slices * std::log2f(mboitPushConstants.zNear) / std::log2f(mboitPushConstants.zFar / mboitPushConstants.zNear));

    for (const MeshComponent* meshComponent : meshCompoents)
    {
        const Mesh& mesh = meshComponent->GetMesh();

        for (const auto& meshMaterial : mesh.materials)
        {
            if (meshMaterial.material->materialRenderMode == MaterialRenderMode::Transparent)
            {
                if (meshMaterial.material->materialType == MaterialType::PBRParameters)
                {
                    mboitStaticMeshPBRParametersPipeline.RenderAbsorbance(secondaryCommandBuffer, meshComponent, meshMaterial, mboitPushConstants);
                }
                else if (meshMaterial.material->materialType == MaterialType::PBRTextures)
                {
                    mboitStaticMeshPBRTexturePipeline.RenderAbsorbance(secondaryCommandBuffer, meshComponent, meshMaterial, mboitPushConstants, sampler);
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

void MBOIT::RenderSceneTransmittanceClustered(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, const Scene& scene, size_t imageIndex, const UniformBuffer<PointLightInfo>& pointLightInfoBuffer, const StorageBuffer<CulledLightsPerCluster>& culledLightsPerCluster, size_t cluster_size_x, size_t cluster_x_count, size_t cluster_y_count, size_t cluster_z_slices)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = transmittanceRenderPass;
    renderPassInfo.framebuffer = transmittanceFrameBuffer[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = transmittanceRenderPass;
    inheritanceInfo.framebuffer = transmittanceFrameBuffer[imageIndex];

    VkDevice device = logicalDevice.GetVkDevice();

    //TODO: Having this many secondary command buffers is bad for performance
    //Cut it down to one seccondary command buffer per thread
    std::vector<VkCommandBuffer> secondaryCommandBuffers;

    CameraComponent* camera = scene.GetActiveCamera();
    const auto meshCompoents = scene.renderingScene.GetSceneMeshes(camera->GetViewFrustum());

    VkCommandBuffer secondaryCommandBuffer = commandPool.GetSecondaryCommandBuffer(logicalDevice, &inheritanceInfo);
    secondaryCommandBuffers.push_back(secondaryCommandBuffer);

    //TODO Set push constants
     //Push constants
    MBOITTransmittanceClusteredConstants mboitTransmittanceConstants;
    mboitTransmittanceConstants.moment_bias = bias;// 5 * 1e-7;
    mboitTransmittanceConstants.overestimation = overestimation;
    mboitTransmittanceConstants.screenWidth = extent.width;
    mboitTransmittanceConstants.screenHeight = extent.height;

    mboitTransmittanceConstants.clusterCounts = { ClusteredLightCulling::cluster_x_count, ClusteredLightCulling::cluster_y_count, ClusteredLightCulling::cluster_z_slices };
    mboitTransmittanceConstants.clusterSizeX = ClusteredLightCulling::cluster_size_x;
    mboitTransmittanceConstants.zNear = camera->GetNearPlane();
    mboitTransmittanceConstants.zFar = camera->GetFarPlane();

    //Basically reduced a log function into a simple multiplication an addition by pre-calculating these
    mboitTransmittanceConstants.scale = (float)ClusteredLightCulling::cluster_z_slices / std::log2f(mboitTransmittanceConstants.zFar / mboitTransmittanceConstants.zNear);
    mboitTransmittanceConstants.bias = -((float)ClusteredLightCulling::cluster_z_slices * std::log2f(mboitTransmittanceConstants.zNear) / std::log2f(mboitTransmittanceConstants.zFar / mboitTransmittanceConstants.zNear));

    mboitTransmittanceConstants.enableEnviromentLight = GlobalOptions::GetInstace().environmentLight;

    const EnvironmentMap& environmentMap = *scene.GetEnvironmentMap();
    VkDescriptorSet perTransmittancePassDescriptorSet = trasnmittancePassClusteredDescriptorSetManager.GetDescriptorSet(imageIndex);
    if (perTransmittancePassDescriptorSet == VK_NULL_HANDLE)
    {
        perTransmittancePassDescriptorSet = trasnmittancePassClusteredDescriptorSetManager.AllocateDescriptorSet(imageIndex);

        auto pointLightBufferInfo = pointLightInfoBuffer.GetDescriptorInfo();
        auto light_visibility_buffer_info = culledLightsPerCluster.GetDescriptorInfo();

        VkDescriptorBufferInfo perFrameBufferInfoBufferInfo = perFrameInfoBuffer.GetDescriptorInfo();
        auto b0Info = b0[imageIndex].GetImageInfo(integrationMapSampler);
        auto b1234Info = b1234[imageIndex].GetImageInfo(integrationMapSampler);
        auto b56Info = b56[imageIndex].GetImageInfo(integrationMapSampler);

        VkDescriptorImageInfo raddianceMapImageInfo = environmentMap.raddianceMap.GetImageInfo(enviromentMapSampler);
        VkDescriptorImageInfo irraddianceMapImageInfo = environmentMap.irraddianceMap.GetImageInfo(enviromentMapSampler);
        VkDescriptorImageInfo integrationMapImageInfo = environmentMap.integrationMap.GetImageInfo(integrationMapSampler);

        DescriptorSetBuilder()
            .BindBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perFrameBufferInfoBufferInfo)
            .BindBuffer(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &pointLightBufferInfo)
            .BindBuffer(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &light_visibility_buffer_info)
            .BindImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &b0Info)
            .BindImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &b1234Info)
            .BindImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &b56Info)
            .BindImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &raddianceMapImageInfo)
            .BindImage(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &irraddianceMapImageInfo)
            .BindImage(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &integrationMapImageInfo)
            .Build(perTransmittancePassDescriptorSet, trasnmittancePassClusteredDescriptorSetManager.GetDeviceHandle());
    }

    for (const MeshComponent* meshComponent : meshCompoents)
    {
        const Mesh& mesh = meshComponent->GetMesh();

        for (const auto& meshMaterial : mesh.materials)
        {
            if (meshMaterial.material->materialRenderMode == MaterialRenderMode::Transparent)
            {
                if (meshMaterial.material->materialType == MaterialType::PBRParameters)
                {
                    mboitStaticMeshPBRParametersPipeline.RenderTransmittanceClustered(secondaryCommandBuffer, meshComponent, meshMaterial, mboitTransmittanceConstants, perTransmittancePassDescriptorSet);
                }
                else if (meshMaterial.material->materialType == MaterialType::PBRTextures)
                {
                    mboitStaticMeshPBRTexturePipeline.RenderTransmittanceClustered(secondaryCommandBuffer, meshComponent, meshMaterial, mboitTransmittanceConstants, perTransmittancePassDescriptorSet, sampler);
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

void MBOIT::CompositieScene(const LogicalDevice& logicalDevice, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& opaqueMeshes, size_t imageIndex)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline);

    VkDescriptorSet texturesDescriptorSet = compositeDescriptorSetManager.GetDescriptorSet(imageIndex);
    if (texturesDescriptorSet == VK_NULL_HANDLE)
    {
        texturesDescriptorSet = compositeDescriptorSetManager.AllocateDescriptorSet(imageIndex);
        VkDescriptorImageInfo opaqueMeshesInfo = opaqueMeshes.GetImageInfo(integrationMapSampler);
        VkDescriptorImageInfo transparentMeshesInfo = transparentMeshes[imageIndex].GetImageInfo(integrationMapSampler);

        VkDescriptorImageInfo b0Info = b0[imageIndex].GetImageInfo(integrationMapSampler);

        DescriptorSetBuilder()
            .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &opaqueMeshesInfo)
            .BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &transparentMeshesInfo)
            .BindImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &b0Info)
            .Build(texturesDescriptorSet, compositeDescriptorSetManager.GetDeviceHandle());
    }  
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipelineLayout, 0, 1, &texturesDescriptorSet, 0, nullptr);

    VkBuffer vertexBuffers[] = { compositeVertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, compositeIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

void MBOIT::CreateAbsorancePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkAttachmentDescription b0Attachment{};
    b0Attachment.format = VK_FORMAT_R32_SFLOAT;
    b0Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    b0Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    b0Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    b0Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    b0Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    b0Attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    b0Attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.format = graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat();
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencyDepth{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkSubpassDependency, 3> dependencies =
    {
        dependency,
        dependency,
        dependency
    };

    std::array<VkAttachmentDescription, 4> attachments =
    {
        b0Attachment,
        colorAttachment,
        colorAttachment,
        depthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkDevice device = logicalDevice.GetVkDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &absoranceRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

    VkExtent2D swapChainExtent = swapChain.GetExtent();

    //Images
    

    // Frame buffers
    absoranceFrameBuffer.resize(swapChain.GetImageCount());
    b0.resize(swapChain.GetImageCount());
    b1234.resize(swapChain.GetImageCount());
    b56.resize(swapChain.GetImageCount());

    for (size_t i = 0; i < absoranceFrameBuffer.size(); i++)
    {
        b0[i] = CreateImage(graphicsAPI, commandPool, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapChainExtent);
        b1234[i] = CreateImage(graphicsAPI, commandPool, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapChainExtent);
        b56[i] = CreateImage(graphicsAPI, commandPool, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapChainExtent);

        std::vector<VkImageView> fbAttachments =
        {
            b0[i].textureImageView,
            b1234[i].textureImageView,
            b56[i].textureImageView,
            depthImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = absoranceRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        framebufferInfo.pAttachments = fbAttachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice.GetVkDevice(), &framebufferInfo, nullptr, &absoranceFrameBuffer[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }    
}

void MBOIT::CreateTransmittancePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, std::vector<VkImageView> depthImageViews)
{
    perFrameInfoBuffer.Create(graphicsAPI);

    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.GetImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.format = graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat();
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentReference> colorReferences =
    {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;

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
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &transmittanceRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

    VkExtent2D swapChainExtent = swapChain.GetExtent();

    //Images

    // Frame buffers
    transmittanceFrameBuffer.resize(swapChain.GetImageCount());
    transparentMeshes.resize(swapChain.GetImageCount());
    for (size_t i = 0; i < transmittanceFrameBuffer.size(); i++)
    {
        transparentMeshes[i] = CreateImage(graphicsAPI, commandPool, swapChain.GetImageFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapChainExtent);
        std::vector<VkImageView> fbAttachments =
        {
            transparentMeshes[i].textureImageView,
            depthImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = transmittanceRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        framebufferInfo.pAttachments = fbAttachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice.GetVkDevice(), &framebufferInfo, nullptr, &transmittanceFrameBuffer[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
       
    CreateSamplers(logicalDevice);

    DescriptorSetLayoutBuilder perTrasnmittancePassLayoutBuilder;
    perTrasnmittancePassLayoutBuilder
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &trasnmittancePassDescriptorSetLayout);

    trasnmittancePassClusteredDescriptorSetManager.Create(logicalDevice, trasnmittancePassDescriptorSetLayout, perTrasnmittancePassLayoutBuilder.GetLayoutBindings());
}

void MBOIT::CreateCompositePass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool, VkRenderPass compositeRenderPass)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    const std::vector <ScreenVertex> vertices =
    {
        ScreenVertex({-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}),
        ScreenVertex({-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
        ScreenVertex({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
        ScreenVertex({1.0f, -1.0f, 1.0f}, {1.0f, 1.0f})
    };

    compositeVertexBuffer.Create(graphicsAPI, commandPool, vertices);

    const std::vector<uint32_t>  indices =
    {
        0, 1, 2,
        0, 2, 3,
    };

    compositeIndexBuffer.Create(graphicsAPI, commandPool, indices);

    VkDevice device = logicalDevice.GetVkDevice();
    VkExtent2D swapChainExtent = swapChain.GetExtent();

    DescriptorSetLayoutBuilder compositeBuilder;
    compositeBuilder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &compositeDescriptorSetLayout);

    compositeDescriptorSetManager.Create(logicalDevice, compositeDescriptorSetLayout, compositeBuilder.GetLayoutBindings());

    std::vector<VkDescriptorSetLayout> layouts
    {
        compositeDescriptorSetLayout,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &compositePipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    //Pipeline
    GraphicsPipelineBuilder pipelineBuilder(logicalDevice);
    pipelineBuilder
        .SetVertexShader("shaders/Common/FullscreenRender.vert.spv", "main")
        .SetFragmentShader("shaders/Transparency/MBOIT/CompositePass.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, { 1.0, 1.0, 1.0, 1.0 })
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(compositeRenderPass, 0)
        .SetPipelineLayout(compositePipelineLayout)
        .Build(compositePipeline, nullptr);
}

void MBOIT::CreateSamplers(const LogicalDevice& logicalDevice)
{
    VkDevice device = logicalDevice.GetVkDevice();

    {
        // CreateCulling sampler
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = 6; //TODO Fix this
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler.anisotropyEnable = VK_TRUE;
        sampler.maxAnisotropy = 16;

        if (vkCreateSampler(device, &sampler, nullptr, &enviromentMapSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    {
        // CreateCulling sampler
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = 1; //TODO Fix this
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.maxAnisotropy = 1;

        if (vkCreateSampler(device, &sampler, nullptr, &integrationMapSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
}

Image MBOIT::CreateImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkFormat format, VkImageUsageFlagBits usage, VkExtent2D viewportExtent)
{
    Image image;

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = viewportExtent.width;
    imageInfo.extent.height = viewportExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &image.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    //Sasha doesnt do this?
    //graphicsAPI.TransitionImageLayoutImmediate(commandPool, queue, image.textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

    return image;
}

#include "HorizonBasedAmbientOcclusion.h"

#include "Vertex.h"

#include "DescriptorSetLayoutBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"

#include "CameraComponent.h"

#include <glm/gtc/random.hpp>

HBAOParameters HorizonBasedAmbientOcclusion::gtaoParameters;
bool HorizonBasedAmbientOcclusion::enable = true;

void HorizonBasedAmbientOcclusion::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D pViewportSize)
{
    viewportSize = { uint32_t(pViewportSize.width / 4.0), uint32_t(pViewportSize.height / 4.0) };
    noiseTextureSize = { 4, 4 };

    CreateFullscreenQuad(graphicsAPI, commandPool);
    CreateAmbientOcclusionMap(graphicsAPI, commandPool, viewportSize);
    CreateRandomNoiseTexture(graphicsAPI, commandPool);
    CreateRenderPass(graphicsAPI, viewportSize);
    CreatePipeline(graphicsAPI, viewportSize);

    ambientOcclusionBlur.Create(graphicsAPI, commandPool, viewportSize, ambientOcclusionMap);
}

void HorizonBasedAmbientOcclusion::RenderAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, const GBuffer& gBuffer, CameraComponent* camera)
{
    gtaoParametersBuffer.Update(gtaoParameters);

    std::array<VkClearValue, 1> clearValues{};
    if (enable)
    {
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    }
    else
    {
        clearValues[0].color = { 1.0f, 0.0f, 0.0f, 0.0f };
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = viewportSize;
    renderPassInfo.clearValueCount = 1;// static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &perLightingPassDescriptorSet, 0, nullptr);

    VkDescriptorSet gBufferInfoDescriptorSet = descriptorSetManager.GetDescriptorSet(this);
    if (gBufferInfoDescriptorSet == VK_NULL_HANDLE)
    {
        gBufferInfoDescriptorSet = descriptorSetManager.AllocateDescriptorSet(this);

        auto depth = gBuffer.depth.image.GetImageInfo(gBuffer.gBufferSampler);
        auto noise = noiseTexture.GetImageInfo(gBuffer.gBufferSampler);
        auto gtaoParametersBufferInfo = gtaoParametersBuffer.GetDescriptorInfo();

        DescriptorSetBuilder builder;
        builder
            .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depth)
            .BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &noise)
            .BindBuffer(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &gtaoParametersBufferInfo)
            .Build(gBufferInfoDescriptorSet, graphicsAPI.GetLogicalDevice().GetVkDevice());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &gBufferInfoDescriptorSet, 0, nullptr);

    HBAOPushConstantObject gtaoPushConstantObject;
    gtaoPushConstantObject.inverseProjection = glm::inverse(camera->GetProjection());

    gtaoPushConstantObject.FocalLen[0] = 1.0f / tanf(camera->GetFOV() * 0.5f) * ((float)viewportSize.height / (float)viewportSize.width);
    gtaoPushConstantObject.FocalLen[1] = 1.0f / tanf(camera->GetFOV() * 0.5f);

    glm::vec2 InvFocalLen;
    InvFocalLen[0] = 1.0f / gtaoPushConstantObject.FocalLen[0];
    InvFocalLen[1] = 1.0f / gtaoPushConstantObject.FocalLen[1];

    gtaoPushConstantObject.UVToViewA[0] = -2.0f * InvFocalLen[0];
    gtaoPushConstantObject.UVToViewA[1] = -2.0f * InvFocalLen[1];
    gtaoPushConstantObject.UVToViewB[0] = 1.0f * InvFocalLen[0];
    gtaoPushConstantObject.UVToViewB[1] = 1.0f * InvFocalLen[1];

    float near = camera->GetNearPlane();
    float far = camera->GetFarPlane();
    gtaoPushConstantObject.LinMAD[0] = (near - far) / (2.0f * near * far);
    gtaoPushConstantObject.LinMAD[1] = (near + far) / (2.0f * near * far);

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(HBAOPushConstantObject), &gtaoPushConstantObject);

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    if (enable)
    {
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    ambientOcclusionBlur.BlurAOMap(graphicsAPI, commandPool, commandBuffer, ambientOcclusionMap);
}

void HorizonBasedAmbientOcclusion::CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    const std::vector <ScreenVertex> vertices =
    {
        ScreenVertex({-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}),
        ScreenVertex({-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}),
        ScreenVertex({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
        ScreenVertex({1.0f, -1.0f, 1.0f}, {1.0f, 1.0f})
    };

    fullscreenQuadVertices.Create(graphicsAPI, commandPool, vertices);

    const std::vector<uint32_t>  indices =
    {
        0, 1, 2,
        0, 2, 3,
    };

    fullscreenQuadIndices.Create(graphicsAPI, commandPool, indices);
}

void HorizonBasedAmbientOcclusion::CreateRenderPass(const GraphicsAPI& graphicsAPI, VkExtent2D viewportSize)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    //Create render pass
    VkAttachmentReference aoMapAttachmentRef{};
    aoMapAttachmentRef.attachment = 0;
    aoMapAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &aoMapAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription aoMapAttachment{};
    aoMapAttachment.format = VkFormat::VK_FORMAT_R32_SFLOAT;
    aoMapAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    aoMapAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    aoMapAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    aoMapAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    aoMapAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    aoMapAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    aoMapAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentDescription, 1> attachments = { aoMapAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

    std::array<VkImageView, 1> frameBufferAttachments =
    {
        ambientOcclusionMap.textureImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(frameBufferAttachments.size());
    framebufferInfo.pAttachments = frameBufferAttachments.data();
    framebufferInfo.width = viewportSize.width;
    framebufferInfo.height = viewportSize.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void HorizonBasedAmbientOcclusion::CreatePipeline(const GraphicsAPI& graphicsAPI, VkExtent2D viewportSize)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &descriptorSetLayout);

    descriptorSetManager.Create(logicalDevice, descriptorSetLayout, builder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(descriptorSetLayout)
        .AddPushConstant(sizeof(HBAOPushConstantObject), VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(pipelineLayout);

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewportSize.width;
    viewport.height = (float)viewportSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = viewportSize;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    //Create pipeline
    GraphicsPipelineBuilder(logicalDevice)
        .SetVertexShader("shaders/Deferred/LightingPass.vert.spv", "main")
        .SetFragmentShader("shaders/AO/HorizonBasedAmbientOcclusion.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        //.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .AddScissor(scissor)
        //.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VkLogicOp::VK_LOGIC_OP_NO_OP, { 1.0, 1.0, 1.0, 1.0 })
        .SetPipelineLayout(pipelineLayout)
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(renderPass, 0)
        .Build(pipeline, nullptr);

    gtaoParametersBuffer.Create(graphicsAPI);
   
    HBAOParameters gtaoParameters;
    gtaoParameters.AORes = { viewport.width, viewport.height };
    gtaoParameters.InvAORes = { 1.0f / viewport.width, 1.0f / viewport.height };

    gtaoParameters.NoiseScale = { viewport.width / noiseTextureSize.width, viewport.height / noiseTextureSize.height };  
    gtaoParameters.AOStrength = 1.0;// 1.9;
    
    gtaoParameters.R = 4.6;// 0.3;
    gtaoParameters.R2 = gtaoParameters.R * gtaoParameters.R;
    gtaoParameters.NegInvR2 = -1.0f / gtaoParameters.R2;

    gtaoParameters.TanBias = std::tan(30.0f * glm::pi<float>() / 180.0f);
    gtaoParameters.MaxRadiusPixels = 70;

    gtaoParameters.NumDirections = 24;// 6;
    gtaoParameters.NumSamples = 10;// 3;

    HorizonBasedAmbientOcclusion::gtaoParameters = gtaoParameters;

    gtaoParametersBuffer.Update(gtaoParameters);
}

void HorizonBasedAmbientOcclusion::CreateAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportSize)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = viewportSize.width;
    imageInfo.extent.height = viewportSize.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &ambientOcclusionMap.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, ambientOcclusionMap.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &ambientOcclusionMap.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, ambientOcclusionMap.textureImage, ambientOcclusionMap.textureImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = ambientOcclusionMap.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &ambientOcclusionMap.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void HorizonBasedAmbientOcclusion::CreateRandomNoiseTexture(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    std::vector<float> noise(noiseTextureSize.width * noiseTextureSize.height * 4);
    for (int y = 0; y < noiseTextureSize.height; ++y)
    {
        for (int x = 0; x < noiseTextureSize.width; ++x)
        {
            glm::vec2 xy = glm::circularRand(1.0f);
            float z = glm::linearRand(0.0f, 1.0f);
            float w = glm::linearRand(0.0f, 1.0f);

            int offset = 4 * (y * noiseTextureSize.width + x);
            noise[offset + 0] = xy[0];
            noise[offset + 1] = xy[1];
            noise[offset + 2] = z;
            noise[offset + 3] = w;
        }
    }

    //Create image
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = noiseTextureSize.width;
    imageInfo.extent.height = noiseTextureSize.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &noiseTexture.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, noiseTexture.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &noiseTexture.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, noiseTexture.textureImage, noiseTexture.textureImageMemory, 0);

    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(memRequirements.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.SetContents(noise.data(), static_cast<size_t>(noise.size() * sizeof(float)));

    //Copy over data
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, noiseTexture.textureImage, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, noiseTexture.textureImage, static_cast<uint32_t>(noiseTextureSize.width), static_cast<uint32_t>(noiseTextureSize.height));
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, noiseTexture.textureImage, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = noiseTexture.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &noiseTexture.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    stagingBuffer.Destroy();
}

#include "AmbientOcclusionBlur.h"

#include "DescriptorSetLayoutBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"
#include "Vertex.h"

void AmbientOcclusionBlur::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportSize, Image& aoMap)
{
    aoMapSize = viewportSize;

    CreateFullscreenQuad(graphicsAPI, commandPool);
    CreatePingPongImage(graphicsAPI, commandPool);
    CreateRenderPass(graphicsAPI, aoMap);
    CreatePipeline(graphicsAPI);
}

void AmbientOcclusionBlur::BlurAOMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& aoMap)
{
    BlurX(graphicsAPI, commandPool, commandBuffer, aoMap);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    graphicsAPI.TransitionImageLayout(commandBuffer, aoMap.textureImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresourceRange);

    BlurY(graphicsAPI, commandPool, commandBuffer);
}

void AmbientOcclusionBlur::CreateFullscreenQuad(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
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

void AmbientOcclusionBlur::CreatePingPongImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = aoMapSize.width;
    imageInfo.extent.height = aoMapSize.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &pingPongImage.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, pingPongImage.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &pingPongImage.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, pingPongImage.textureImage, pingPongImage.textureImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = pingPongImage.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &pingPongImage.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void AmbientOcclusionBlur::CreateRenderPass(const GraphicsAPI& graphicsAPI, Image& aoMap)
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

    {
        std::array<VkImageView, 1> frameBufferAttachments =
        {
            aoMap.textureImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(frameBufferAttachments.size());
        framebufferInfo.pAttachments = frameBufferAttachments.data();
        framebufferInfo.width = aoMapSize.width;
        framebufferInfo.height = aoMapSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &aoMapBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
   
    {
        std::array<VkImageView, 1> frameBufferAttachments =
        {
            pingPongImage.textureImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(frameBufferAttachments.size());
        framebufferInfo.pAttachments = frameBufferAttachments.data();
        framebufferInfo.width = aoMapSize.width;
        framebufferInfo.height = aoMapSize.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &pingPongBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void AmbientOcclusionBlur::CreatePipeline(const GraphicsAPI& graphicsAPI)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &descriptorSetLayout);

    blurXdescriptorSetManager.Create(logicalDevice, descriptorSetLayout, builder.GetLayoutBindings());
    blurYdescriptorSetManager.Create(logicalDevice, descriptorSetLayout, builder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(descriptorSetLayout)
        .AddPushConstant(sizeof(AOBlurParameters), VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(pipelineLayout);

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)aoMapSize.width;
    viewport.height = (float)aoMapSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = aoMapSize;

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
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    std::array < VkPipelineColorBlendAttachmentState, 1 > colorBlendAttachments =
    {
        colorBlendAttachment,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

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
        .SetFragmentShader("shaders/Blur/Blur.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VkLogicOp::VK_LOGIC_OP_NO_OP, { 1.0, 1.0, 1.0, 1.0 })
        .SetPipelineLayout(pipelineLayout)
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(renderPass, 0)
        .Build(pipeline, nullptr);
}

void AmbientOcclusionBlur::BlurX(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer, Image& aoMap)
{
    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
  
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = pingPongBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = aoMapSize;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDescriptorSet aoMapInfoDescriptorSet = blurXdescriptorSetManager.GetDescriptorSet(this);
    if (aoMapInfoDescriptorSet == VK_NULL_HANDLE)
    {
        aoMapInfoDescriptorSet = blurXdescriptorSetManager.AllocateDescriptorSet(this);

        auto aoMapInfo = aoMap.GetImageInfo(graphicsAPI.GetPointSampler());

        DescriptorSetBuilder builder;
        builder
            .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoMapInfo)
            .Build(aoMapInfoDescriptorSet, graphicsAPI.GetLogicalDevice().GetVkDevice());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &aoMapInfoDescriptorSet, 0, nullptr);

    AOBlurParameters aoBlurParameters;
    aoBlurParameters.horizontal = true;
    aoBlurParameters.AORes = { aoMapSize.width, aoMapSize.height };
    aoBlurParameters.InvAORes = { 1.0f / aoMapSize.width, 1.0f / aoMapSize.height };

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AOBlurParameters), &aoBlurParameters);

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void AmbientOcclusionBlur::BlurY(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkCommandBuffer commandBuffer)
{
    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = aoMapBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = aoMapSize;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDescriptorSet aoMapInfoDescriptorSet = blurYdescriptorSetManager.GetDescriptorSet(this);
    if (aoMapInfoDescriptorSet == VK_NULL_HANDLE)
    {
        aoMapInfoDescriptorSet = blurYdescriptorSetManager.AllocateDescriptorSet(this);

        auto pingPongImageInfo = pingPongImage.GetImageInfo(graphicsAPI.GetPointSampler());

        DescriptorSetBuilder builder;
        builder
            .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &pingPongImageInfo)
            .Build(aoMapInfoDescriptorSet, graphicsAPI.GetLogicalDevice().GetVkDevice());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &aoMapInfoDescriptorSet, 0, nullptr);

    AOBlurParameters aoBlurParameters;
    aoBlurParameters.horizontal = false;
    aoBlurParameters.AORes = { aoMapSize.width, aoMapSize.height };
    aoBlurParameters.InvAORes = { 1.0f / aoMapSize.width, 1.0f / aoMapSize.height };

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AOBlurParameters), &aoBlurParameters);

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

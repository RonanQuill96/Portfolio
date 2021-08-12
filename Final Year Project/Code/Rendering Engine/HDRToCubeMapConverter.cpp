#include "HDRToCubeMapConverter.h"


#include "Vertex.h"

#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"
#include "DescriptorSetLayoutBuilder.h"

#include <array>

struct PushData
{
    glm::mat4 projection;
    glm::mat4 view;
};

Image HDRToCubeMapConverter::ConvertHDRToCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, std::string filename, VkExtent2D textureDimensions)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    ImageData hdrImageData = ImageData::LoadImageFromFile(filename);

    //Convert this to a texture to be able to bind it to a shader
    Image hdrImage = CreateHDRImage(graphicsAPI, commandPool, hdrImageData);

    Image cubeMap = CreateCubeMap(graphicsAPI, commandPool, textureDimensions);
    CreateFrameBufferImage(graphicsAPI, commandPool, textureDimensions);
    CreateRenderPass(logicalDevice);
    CreateRenderTargets(logicalDevice, textureDimensions);
    CreatePipeline(logicalDevice, textureDimensions);
    CreateBuffers(graphicsAPI, commandPool);

    RenderCubeMap(graphicsAPI, commandPool, textureDimensions, cubeMap, hdrImage);

    return cubeMap;
}

Image HDRToCubeMapConverter::CreateHDRImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, ImageData& hdrImageData)
{
    Image hdrImage;

    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(hdrImageData.GetImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.SetContents(hdrImageData.GetDataPtr(), hdrImageData.GetImageSize());

    hdrImageData.FreeImageData();

    graphicsAPI.CreateImage(hdrImageData.GetWidth(), hdrImageData.GetHeight(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, hdrImage.textureImage, hdrImage.textureImageMemory);

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, hdrImage.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, hdrImage.textureImage, static_cast<uint32_t>(hdrImageData.GetWidth()), static_cast<uint32_t>(hdrImageData.GetHeight()));
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, hdrImage.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.Destroy();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = hdrImage.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(graphicsAPI.GetLogicalDevice().GetVkDevice(), &viewInfo, nullptr, &hdrImage.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return hdrImage;
}

Image HDRToCubeMapConverter::CreateCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension)
{
    Image cubemap;

    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    // CreateCulling optimal tiled target image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(textureDimension.width), static_cast<uint32_t>(textureDimension.height), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // Cube faces count as array layers in Vulkan
    imageCreateInfo.arrayLayers = 6;
    // This flag is required for cube map images
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &cubemap.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, cubemap.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &cubemap.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, cubemap.textureImage, cubemap.textureImageMemory, 0);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 6;

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubemap.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    // CreateCulling image view
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // Cube map view type
    view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view.format = VK_FORMAT_R8G8B8A8_SRGB;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    // 6 array layers (faces)
    view.subresourceRange.layerCount = 6;
    // Set number of mip levels
    view.subresourceRange.levelCount = 1;
    view.image = cubemap.textureImage;

    if (vkCreateImageView(device, &view, nullptr, &cubemap.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return cubemap;
}

void HDRToCubeMapConverter::CreateFrameBufferImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension)
{
    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    // CreateCulling optimal tiled target image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(textureDimension.width), static_cast<uint32_t>(textureDimension.height), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.arrayLayers = 1;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &frameBufferImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, frameBufferImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &frameBufferImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, frameBufferImage, frameBufferImageMemory, 0);

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, frameBufferImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // CreateCulling image view
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = VK_FORMAT_R8G8B8A8_SRGB;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    view.subresourceRange.layerCount = 1;
    view.subresourceRange.levelCount = 1;
    view.image = frameBufferImage;

    if (vkCreateImageView(device, &view, nullptr, &frameBufferImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void HDRToCubeMapConverter::CreateRenderPass(const LogicalDevice& logicalDevice)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB; //TODO probably need to change
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkDevice device = logicalDevice.GetVkDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void HDRToCubeMapConverter::CreateRenderTargets(const LogicalDevice& logicalDevice, VkExtent2D textureDimension)
{
    VkDevice device = logicalDevice.GetVkDevice();

    std::array<VkImageView, 1> fbattachments =
    {
        frameBufferImageView,
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(fbattachments.size());
    framebufferInfo.pAttachments = fbattachments.data();
    framebufferInfo.width = textureDimension.width;
    framebufferInfo.height = textureDimension.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void HDRToCubeMapConverter::CreateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    const std::vector <ScreenVertex> vertices =
    {
        ScreenVertex({0.5f, -0.5f, 0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({-0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f }),
        ScreenVertex({-0.5f, -0.5f, 0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({-0.5f, 0.5f, 0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({0.5f, 0.5f, -0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f }),
        ScreenVertex({0.5f, -0.5f, 0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({0.5f, 0.5f, -0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({-0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f }),
        ScreenVertex({0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({-0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f }),
        ScreenVertex({-0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({-0.5f, -0.5f, 0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({0.5f, -0.5f, 0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({-0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f }),
        ScreenVertex({0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({-0.5f, 0.5f, -0.5f}, { 1.0f, 0.0f }),
        ScreenVertex({0.5f, 0.5f, -0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({-0.5f, 0.5f, -0.5f}, { 1.0f, 1.0f }),
        ScreenVertex({-0.5f, 0.5f, -0.5f}, { 0.0f, 0.0f }),
        ScreenVertex({-0.5f, -0.5f, 0.5f}, { 1.0f, 0.0f })
    };

    vertexBuffer.Create(graphicsAPI, commandPool, vertices);

    const std::vector<uint32_t>  indices =
    {
        0, 2, 1,
        3, 5, 4,
        6, 8, 7,
        9, 11, 10,
        12, 14, 13,
        15, 17, 16,
        0, 1, 18,
        3, 4, 19,
        6, 7, 20,
        9, 10, 21,
        12, 13, 22,
        15, 16, 23,
    };

    indexCount = indices.size();

    indexBuffer.Create(graphicsAPI, commandPool, indices);
}

void HDRToCubeMapConverter::CreatePipeline(const LogicalDevice& logicalDevice, VkExtent2D textureDimension)
{
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &descriptorSetLayout);
    descriptorSetManager.Create(logicalDevice, descriptorSetLayout, builder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(descriptorSetLayout)
        .AddPushConstant(sizeof(PushData), VK_SHADER_STAGE_VERTEX_BIT)
        .Build(pipelineLayout);

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

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
    depthStencil.depthWriteEnable = VK_FALSE;// VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)textureDimension.width;
    viewport.height = (float)textureDimension.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { textureDimension.width, textureDimension.height };

    GraphicsPipelineBuilder(logicalDevice)
        .SetVertexShader("shaders/Skybox/HDRToCubemap/hdr-to-cubemap.vert.spv", "main")
        .SetFragmentShader("shaders/Skybox/HDRToCubemap/hdr-to-cubemap.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VkLogicOp::VK_LOGIC_OP_COPY, { 1.0, 1.0, 1.0, 1.0 })
        .SetPipelineLayout(pipelineLayout)
        .DisableDepth()
        .SetRenderPass(renderPass, 0)
        .Build(pipeline, nullptr);
}

void HDRToCubeMapConverter::RenderCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D textureDimension, Image& cubeMap, Image& hdrImage)
{
    VkCommandBuffer commandBuffer = commandPool.GetPrimaryCommandBuffer(graphicsAPI.GetLogicalDevice());

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = frameBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = textureDimension;

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    //clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = renderPass;
    // Secondary command buffer also uses the currently active framebuffer
    inheritanceInfo.framebuffer = frameBuffer;

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    captureProjection[1][1] *= -1.0f;

    glm::mat4 captureViews[] =
    {
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptorSet = descriptorSetManager.GetDescriptorSet(this);
        if (descriptorSet == VK_NULL_HANDLE)
        {
            descriptorSet = descriptorSetManager.AllocateDescriptorSet(this);

            auto hdrImageInfo = hdrImage.GetImageInfo(graphicsAPI.GetDefaultTextureSampler());
            DescriptorSetBuilder()
                .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &hdrImageInfo)
                .Build(descriptorSet, descriptorSetManager.GetDeviceHandle());
        }
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        PushData pushData;
        pushData.projection = captureProjection;
        pushData.view = captureViews[faceIndex];
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushData), &pushData);

        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        //Now copy from framebuffer to cube face
        //https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingomni/shadowmappingomni.cpp
        VkImageSubresourceRange fbsubresourceRange{};
        fbsubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        fbsubresourceRange.baseMipLevel = 0;
        fbsubresourceRange.levelCount = 1;
        fbsubresourceRange.layerCount = 1;
        graphicsAPI.TransitionImageLayout(commandBuffer, frameBufferImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, fbsubresourceRange);

        VkImageSubresourceRange imsubresourceRange{};
        imsubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imsubresourceRange.baseMipLevel = 0;
        imsubresourceRange.levelCount = 1;
        imsubresourceRange.layerCount = 1;
        imsubresourceRange.baseArrayLayer = faceIndex;
        graphicsAPI.TransitionImageLayout(commandBuffer, cubeMap.textureImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imsubresourceRange);

        // Copy region for transfer from framebuffer to cube face
        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = { 0, 0, 0 };

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = faceIndex;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = { 0, 0, 0 };

        copyRegion.extent.width = textureDimension.width;
        copyRegion.extent.height = textureDimension.height;
        copyRegion.extent.depth = 1;

        // Put image copy into command buffer
        vkCmdCopyImage(
            commandBuffer,
            frameBufferImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            cubeMap.textureImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        graphicsAPI.TransitionImageLayout(commandBuffer, frameBufferImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, fbsubresourceRange);
        graphicsAPI.TransitionImageLayout(commandBuffer, cubeMap.textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imsubresourceRange);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    //submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(graphicsAPI.GetLogicalDevice().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit irradiance map queue!");
    }

    if (graphicsAPI.GetLogicalDevice().WaitIdle() != VK_SUCCESS)
    {
        throw std::runtime_error("failed to wait!");
    }
    //commandPool.FreeSecondaryCommandBuffer(std::move(commandBuffer));
}

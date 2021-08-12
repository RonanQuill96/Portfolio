#include "UIRenderer.h"

#include "DescriptorSetLayoutBuilder.h"
#include "ShaderUtils.h"

#include "GraphicsPipelineBuilder.h"

void UIRenderer::Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent, CommandPool& commandPool, VkRenderPass renderPass)
{
	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(viewportExtent.width, viewportExtent.height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    m_device = graphicsAPI.GetLogicalDevice().GetVkDevice();

	//Copy font to image
    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.SetContents(fontData, static_cast<size_t>(uploadSize));

    graphicsAPI.CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, font.textureImage, font.textureImageMemory);

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, font.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, font.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, font.textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    stagingBuffer.Destroy();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = font.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &font.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    io.Fonts->TexID = (ImTextureID)(intptr_t)font.textureImage;

    //Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.minLod = -1000;
    samplerInfo.maxLod = 1000;
    samplerInfo.maxAnisotropy = 1.0f;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }

    //Descriptor
    DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
    descriptorSetLayoutBuilder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(m_device, &descriptorSetLayout);

    descriptorSetManager.Create(graphicsAPI.GetLogicalDevice(), descriptorSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

    VkDescriptorImageInfo imageInfo = font.GetImageInfo(sampler);

    descriptorSet = descriptorSetManager.AllocateDescriptorSet(this);

    DescriptorSetBuilder()
        .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo)
        .Build(descriptorSet, m_device);


    VkDescriptorSetLayout layouts[]
    {
        descriptorSetLayout,
    };

    VkPushConstantRange push_constant_range = {};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(PushConstants);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &push_constant_range;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    //Pipeline
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();


    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);

    VkVertexInputBindingDescription inputDesc{};
    inputDesc.binding = 0;
    inputDesc.stride = sizeof(ImDrawVert);
    inputDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription pos{};
    pos.location = 0;
    pos.binding = 0;
    pos.format = VK_FORMAT_R32G32_SFLOAT;
    pos.offset = offsetof(ImDrawVert, pos);

    VkVertexInputAttributeDescription uv{};
    uv.location = 1;
    uv.binding = 0;
    uv.format = VK_FORMAT_R32G32_SFLOAT;
    uv.offset = offsetof(ImDrawVert, uv);

    VkVertexInputAttributeDescription col{};
    col.location = 2;
    col.binding = 0;
    col.format = VK_FORMAT_R8G8B8A8_UNORM;
    col.offset = offsetof(ImDrawVert, col);

    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
    {
        pos, uv, col
    };

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewportExtent.width;
    viewport.height = (float)viewportExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = viewportExtent;

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
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    GraphicsPipelineBuilder graphicsPipelineBuilder(logicalDevice);
    graphicsPipelineBuilder
        .SetVertexShader("shaders/UI/ui.vert.spv", "main")
        .SetFragmentShader("shaders/UI/ui.frag.spv", "main")
        .SetVertexInputState(inputDesc, vertexInputAttributes)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(renderPass, 0)
        .SetPipelineLayout(pipelineLayout)
        .Build(pipeline, pipelineCache);
}


void UIRenderer::UpdateBuffers(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    ImDrawData* imDrawData = ImGui::GetDrawData();

    // Note: Alignment is done inside buffer creation
    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) 
    {
        return;
    }

    // Update buffers only if vertex or index count has been changed compared to current buffer size

        // Vertex buffer
    if ((vertexBuffer.GetBuffer() == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) 
    {
        vertexBuffer.Cleanup();
        std::vector< ImDrawVert> vertices(imDrawData->TotalVtxCount);
        vertexBuffer.Create(graphicsAPI, commandPool, vertices, true);
        vertexCount = imDrawData->TotalVtxCount;
    }

    // Index buffer
    if ((indexBuffer.GetBuffer() == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount))
    {
        indexBuffer.Cleanup();
        std::vector<ImDrawIdx> indices(imDrawData->TotalIdxCount);
        indexBuffer.Create(graphicsAPI, commandPool, indices, true);
        indexCount = imDrawData->TotalIdxCount;
    }

    // Upload data
    VmaAllocator allocator = graphicsAPI.GetAllocator();

    void* vertData;
    void* indexData;
    vmaMapMemory(allocator, vertexBuffer.GetAllocation(), &vertData);
    vmaMapMemory(allocator, indexBuffer.GetAllocation(), &indexData);

    ImDrawVert* vtxDst = (ImDrawVert*)vertData;
    ImDrawIdx* idxDst = (ImDrawIdx*)indexData;

    for (int n = 0; n < imDrawData->CmdListsCount; n++) 
    {
        const ImDrawList* cmd_list = imDrawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }

    vmaUnmapMemory(allocator, vertexBuffer.GetAllocation());
    vmaUnmapMemory(allocator, indexBuffer.GetAllocation());
}

void UIRenderer::DrawCurrentFrame(VkCommandBuffer commandBuffer)
{
    ImGuiIO& io = ImGui::GetIO();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    VkViewport viewport{};
    viewport.width = ImGui::GetIO().DisplaySize.x;
    viewport.height = ImGui::GetIO().DisplaySize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // UI scale and translate via push constants
    PushConstants pushConstBlock{};
    pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstBlock.translation = glm::vec2(-1.0f);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstBlock);

    // Render commands
    ImDrawData* imDrawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    if (imDrawData->CmdListsCount > 0) 
    {
        VkDeviceSize offsets[1] = { 0 };

        VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer() };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);

                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

void UIRenderer::CleanUp()
{
	ImGui::DestroyContext();
}

#include "SkyboxPipeline.h"

#include "GameObject.h"
#include "CameraComponent.h"

#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"

#include "ImageData.h"

void SkyBox::Load(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapLoadInfo& skyBoxLoadInfo)
{
    image = ImageUtils::LoadCubeMap(graphicsAPI, commandPool, skyBoxLoadInfo);

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

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
    sampler.maxLod = 1;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.maxAnisotropy = 1.0f;

    VkPhysicalDeviceProperties properties = graphicsAPI.GetPhysicalDevice().GetProperties();
    sampler.anisotropyEnable = VK_TRUE;
    sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    if (vkCreateSampler(device, &sampler, nullptr, &textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }

    //Geometry
    const std::vector<SkyBoxVertex> vertices =
    {
        // Front Face
        {{-1.0f, -1.0f, -1.0f}},
        {{-1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},

        // Back Face
        {{-1.0f, -1.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}},

        // Top Face
        {{-1.0f, 1.0f, -1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, 1.0f, -1.0f}},

        // Bottom Face
        {{-1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, -1.0f, 1.0f}},
        {{-1.0f, -1.0f, 1.0f}},

        // Left Face
        {{-1.0f, -1.0f, 1.0f}},
        {{-1.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, -1.0f}},
        {{-1.0f, -1.0f, -1.0f}},

        // Right Face
        {{1.0f, -1.0f, -1.0f}},
        {{1.0f, 1.0f, -1.0f}},
        {{1.0f, 1.0f, 1.0f}},
        {{1.0f, -1.0f, 1.0f}},
    };

    vertexBuffer.Create(graphicsAPI, commandPool, vertices);

    std::vector<uint32_t> indices =
    {
        // Front Face
        0, 1, 2,
        0, 2, 3,

        // Back Face
        4, 5, 6,
        4, 6, 7,

        // Top Face
        8, 9, 10,
        8, 10, 11,

        // Bottom Face
        12, 13, 14,
        12, 14, 15,

        // Left Face
        16, 17, 18,
        16, 18, 19,

        // Right Face
        20, 21, 22,
        20, 22, 23
    };

    indexBuffer.Create(graphicsAPI, commandPool, indices);
}

void SkyboxPipeline::Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    CreateDescriptorSetLayout(logicalDevice);

    CreatePipeline(logicalDevice, renderPass, viewportExtent);

    uniformBuffer.Create(graphicsAPI);
}

void SkyboxPipeline::CreatePipeline(const LogicalDevice& logicalDevice, VkRenderPass renderPass, VkExtent2D viewportExtent)
{
    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(descriptorSetLayout)
        .Build(pipelineLayout);

    auto bindingDescription = SkyBoxVertex::GetBindingDescription();
    auto attributeDescriptions = SkyBoxVertex::GetAttributeDescriptions();

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
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    GraphicsPipelineBuilder(logicalDevice)
        .SetVertexShader("shaders/skybox/skybox.vert.spv", "main")
        .SetFragmentShader("shaders/skybox/skybox.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VkLogicOp::VK_LOGIC_OP_NO_OP, { 1.0, 1.0, 1.0, 1.0 })
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(renderPass, 0)
        .SetPipelineLayout(pipelineLayout)
        .Build(pipeline, nullptr);
}

void SkyboxPipeline::CleanUpRenderPassDependentObjects(VkDevice device)
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void SkyboxPipeline::CleanUp(VkDevice device)
{
    skyBoxDescriptorSetManager.CleanUp();
    uniformBuffer.Cleanup();
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void SkyboxPipeline::RenderSkyBox(VkCommandBuffer commandBuffer, VkDevice device, SkyBox* skyBox, CameraComponent* cameraComponent)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    PerSkyBoxBufferInfo info{};
    info.viewProjection = cameraComponent->GetProjection() * glm::mat4(glm::mat3(cameraComponent->GetView()));

    uniformBuffer.Update(info);

    VkBuffer vertexBuffers[] = { skyBox->vertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, skyBox->indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet skyBoxDescriptorSet = skyBoxDescriptorSetManager.GetDescriptorSet(skyBox);
    if (skyBoxDescriptorSet == VK_NULL_HANDLE)
    {
        skyBoxDescriptorSet = CreateDescriptorSet(skyBox);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &skyBoxDescriptorSet, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

VkDescriptorSet SkyboxPipeline::CreateDescriptorSet(SkyBox* skyBox)
{
    VkDescriptorSet descriptorSet = skyBoxDescriptorSetManager.AllocateDescriptorSet(skyBox);

    std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

    VkDescriptorBufferInfo perObjectBufferInfo = uniformBuffer.GetDescriptorInfo();
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].pBufferInfo = &perObjectBufferInfo;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = skyBox->image.textureImageView;
    imageInfo.sampler = skyBox->textureSampler;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstSet = descriptorSet;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(skyBoxDescriptorSetManager.GetDeviceHandle(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    return descriptorSet;
}

void SkyboxPipeline::CreateDescriptorSetLayout(const LogicalDevice& logicalDevice)
{
    VkDevice device = logicalDevice.GetVkDevice();

    //General
    {
        std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings{};

        //Per skybox buffer
        setLayoutBindings[0].binding = 0;
        setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        setLayoutBindings[0].descriptorCount = 1;
        setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        setLayoutBindings[0].pImmutableSamplers = nullptr; // Optional

        //Skybox
        setLayoutBindings[1].binding = 1;
        setLayoutBindings[1].descriptorCount = 1;
        setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setLayoutBindings[1].pImmutableSamplers = nullptr;
        setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = setLayoutBindings.size();
        layoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        skyBoxDescriptorSetManager.Create(logicalDevice, descriptorSetLayout, setLayoutBindings);
    }
}

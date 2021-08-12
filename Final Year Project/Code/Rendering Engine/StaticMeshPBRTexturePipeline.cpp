#include "StaticMeshPBRTexturePipeline.h"

#include "DescriptorSetLayoutBuilder.h"
#include "DescriptorSetBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"

#include "ShaderUtils.h"

void StaticMeshPBRTexturePipeline::Create(const GraphicsAPI& graphicsAPI, VkRenderPass geometryRenderPass, VkExtent2D viewportExtent)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build(device, &descriptorSetLayoutMesh);
    descriptorSetManagerMesh.Create(logicalDevice, descriptorSetLayoutMesh, builder.GetLayoutBindings());

    builder = {};
    builder
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &descriptorSetLayoutMaterial);
    descriptorSetManagerMaterial.Create(logicalDevice, descriptorSetLayoutMaterial, builder.GetLayoutBindings());

    PipelineLayoutBuilder(logicalDevice)
        .AddDescriptorSetLayout(descriptorSetLayoutMesh)
        .AddDescriptorSetLayout(descriptorSetLayoutMaterial)
        .Build(pipelineLayout);

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    GraphicsPipelineBuilder pipelineBuilder(logicalDevice);
    pipelineBuilder
        .SetVertexShader("shaders/pbr.vert.spv", "main")
        .SetFragmentShader("shaders/Deferred/GeometryPassPBRTextures.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, { 1.0, 1.0, 1.0, 1.0 })
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(geometryRenderPass, 0)
        .SetPipelineLayout(pipelineLayout)
        .Build(pipeline, nullptr);

    //Sampler
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void StaticMeshPBRTexturePipeline::CleanUp(VkDevice device)
{
    descriptorSetManagerMesh.CleanUp();
    descriptorSetManagerMaterial.CleanUp();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayoutMesh, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayoutMaterial, nullptr);

    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
}

void StaticMeshPBRTexturePipeline::Render(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial)
{
    const Mesh& mesh = meshComponent->GetMesh();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkBuffer vertexBuffers[] = { mesh.vertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet descriptorSetMesh = descriptorSetManagerMesh.GetDescriptorSet(meshComponent);
    if (descriptorSetMesh == VK_NULL_HANDLE)
    {
        descriptorSetMesh = descriptorSetManagerMesh.AllocateDescriptorSet(meshComponent);

        VkDescriptorBufferInfo perObjectBufferInfo = meshComponent->perObjectBuffer.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
            .Build(descriptorSetMesh, descriptorSetManagerMesh.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetMesh, 0, nullptr);

    VkDescriptorSet descriptorSetMaterial = descriptorSetManagerMaterial.GetDescriptorSet(meshMaterial.material.get());
    if (descriptorSetMaterial == VK_NULL_HANDLE)
    {
        descriptorSetMaterial = descriptorSetManagerMaterial.AllocateDescriptorSet(meshMaterial.material.get());

        PBRTextureMaterial* material = static_cast<PBRTextureMaterial*>(meshMaterial.material.get());
        VkDescriptorImageInfo albedo = material->albedo.GetImageInfo(sampler);
        VkDescriptorImageInfo normals = material->normals.GetImageInfo(sampler);
        VkDescriptorImageInfo metalicRougness = material->metalicRougness.GetImageInfo(sampler);

        DescriptorSetBuilder()
            .BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &albedo)
            .BindImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normals)
            .BindImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metalicRougness)
            .Build(descriptorSetMaterial, descriptorSetManagerMaterial.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSetMaterial, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshMaterial.indexCount), 1, static_cast<uint32_t>(meshMaterial.startIndex), 0, 0);
}
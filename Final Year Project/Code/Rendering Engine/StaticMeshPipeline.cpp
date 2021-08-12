#include "StaticMeshPipeline.h"

#include "Vertex.h"

#include "DescriptorSetLayoutBuilder.h"
#include "ShaderUtils.h"
#include "DescriptorSetBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"

#include <array>

void StaticMeshPBRMaterialsPipeline::Create(const GraphicsAPI& graphicsAPI, VkRenderPass geometryRenderPass, VkExtent2D viewportExtent)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build(device, &meshDescriptorSetLayout);
    meshDescriptorSetManager.Create(logicalDevice, meshDescriptorSetLayout, builder.GetLayoutBindings());

    builder = {};
    builder
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &materialDescriptorSetLayout);
    materialDescriptorSetManager.Create(logicalDevice, materialDescriptorSetLayout, builder.GetLayoutBindings());

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    std::vector<VkDescriptorSetLayout> layouts
    {
        meshDescriptorSetLayout,
        materialDescriptorSetLayout
    };

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

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
        .SetFragmentShader("shaders/Deferred/GeometryPassPBRParameters.frag.spv", "main")
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
}

void StaticMeshPBRMaterialsPipeline::CleanUp(VkDevice device)
{
    materialDescriptorSetManager.CleanUp();
    vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
}

void StaticMeshPBRMaterialsPipeline::Render(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const Mesh& mesh = meshComponent->GetMesh();

    VkBuffer vertexBuffers[] = { mesh.vertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet meshDescriptorSet = meshDescriptorSetManager.GetDescriptorSet(meshComponent);
    if (meshDescriptorSet == VK_NULL_HANDLE)
    {
        meshDescriptorSet = meshDescriptorSetManager.AllocateDescriptorSet(meshComponent);

        VkDescriptorBufferInfo perObjectBufferInfo = meshComponent->perObjectBuffer.GetDescriptorInfo();
        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
            .Build(meshDescriptorSet, meshDescriptorSetManager.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &meshDescriptorSet, 0, nullptr);

    VkDescriptorSet materialDescriptorSet = materialDescriptorSetManager.GetDescriptorSet(meshMaterial.material.get());
    if (materialDescriptorSet == VK_NULL_HANDLE)
    {
        materialDescriptorSet = materialDescriptorSetManager.AllocateDescriptorSet(meshMaterial.material.get());

        PBRParameterMaterial* pbrMaterial = static_cast<PBRParameterMaterial*>(meshMaterial.material.get());

        VkDescriptorBufferInfo pbrMaterialBufferInfoBufferInfo = pbrMaterial->uniformBuffer.GetDescriptorInfo();
        DescriptorSetBuilder()
            .BindBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &pbrMaterialBufferInfoBufferInfo)
            .Build(materialDescriptorSet, materialDescriptorSetManager.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &materialDescriptorSet, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshMaterial.indexCount), 1, static_cast<uint32_t>(meshMaterial.startIndex), 0, 0);
}
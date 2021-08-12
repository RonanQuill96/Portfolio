#include "MBOITStaticMeshPBRParametersPipeline.h"

#include "GraphicsPipelineBuilder.h"
#include "DescriptorSetLayoutBuilder.h"
#include "Vertex.h"

#include "GlobalOptions.h"

#include "ClusteredLightCulling.h"

#include "PipelineLayoutBuilder.h"

void MBOITStaticMeshPBRParametersPipeline::Create(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass absorbanceRenderPass, VkRenderPass transmittanceRenderPass, VkDescriptorSetLayout perTransmittancePassDescriptorSetLayout)
{
    CreateAbsorbancePipeline(graphicsAPI, swapChain, absorbanceRenderPass);
    CreateTrasnmitanceClusteredPipeline(graphicsAPI, swapChain, transmittanceRenderPass, perTransmittancePassDescriptorSetLayout);
}

void MBOITStaticMeshPBRParametersPipeline::RenderAbsorbance(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial,MBOITPushConstants mboitPushConstants)
{  
    const Mesh& mesh = meshComponent->GetMesh();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, absorbancePipeline);

    //Push constants
    vkCmdPushConstants(commandBuffer, absorbancePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MBOITPushConstants), &mboitPushConstants);

    VkBuffer vertexBuffers[] = { mesh.vertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDevice device = absorbanceDescriptorSetManagerMesh.GetDeviceHandle();
    VkDescriptorSet descriptorSetMesh = absorbanceDescriptorSetManagerMesh.GetDescriptorSet(meshComponent);
    if (descriptorSetMesh == VK_NULL_HANDLE)
    {
        descriptorSetMesh = absorbanceDescriptorSetManagerMesh.AllocateDescriptorSet(meshComponent);

        VkDescriptorBufferInfo perObjectBufferInfo = meshComponent->perObjectBuffer.GetDescriptorInfo();
        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
            .Build(descriptorSetMesh, device);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, absorbancePipelineLayout, 0, 1, &descriptorSetMesh, 0, nullptr);

    VkDescriptorSet descriptorSetMaterial = absorbanceDescriptorSetManagerMaterial.GetDescriptorSet(meshMaterial.material.get());
    if (descriptorSetMaterial == VK_NULL_HANDLE)
    {
        descriptorSetMaterial = absorbanceDescriptorSetManagerMaterial.AllocateDescriptorSet(meshMaterial.material.get());

        PBRParameterMaterial* pbrMaterial = static_cast<PBRParameterMaterial*>(meshMaterial.material.get());

        auto albedoImageInfo = pbrMaterial->uniformBuffer.GetDescriptorInfo();;
        DescriptorSetBuilder()
            .BindBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &albedoImageInfo)
            .Build(descriptorSetMaterial, device);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, absorbancePipelineLayout, 1, 1, &descriptorSetMaterial, 0, nullptr);

    VkDescriptorSet descriptorSetCluster = absorbanceDescriptorSetManagerClusters.GetDescriptorSet(this);
    if (descriptorSetCluster == VK_NULL_HANDLE)
    {
        descriptorSetCluster = absorbanceDescriptorSetManagerClusters.AllocateDescriptorSet(this);
        auto clustersInfo = ClusteredLightCulling::clusters.GetDescriptorInfo();
        DescriptorSetBuilder()
            .BindBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &clustersInfo)
            .Build(descriptorSetCluster, device);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, absorbancePipelineLayout, 2, 1, &descriptorSetCluster, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshMaterial.indexCount), 1, static_cast<uint32_t>(meshMaterial.startIndex), 0, 0);
}

void MBOITStaticMeshPBRParametersPipeline::RenderTransmittanceClustered(VkCommandBuffer commandBuffer, const MeshComponent* meshComponent, const MeshMaterial& meshMaterial, MBOITTransmittanceClusteredConstants mboitTransmittanceConstants, VkDescriptorSet perTransmittancePassDescriptor)
{
    const Mesh& mesh = meshComponent->GetMesh();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transmittanceClusteredPipeline);

    vkCmdPushConstants(commandBuffer, transmittancePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MBOITTransmittanceClusteredConstants), &mboitTransmittanceConstants);

    VkBuffer vertexBuffers[] = { mesh.vertexBuffer.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet perMeshDescriptorSet = perMeshDescriptorSetManager.GetDescriptorSet(meshComponent);
    if (perMeshDescriptorSet == VK_NULL_HANDLE)
    {
        perMeshDescriptorSet = perMeshDescriptorSetManager.AllocateDescriptorSet(meshComponent);

        VkDescriptorBufferInfo perObjectBufferInfo = meshComponent->perObjectBuffer.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
            .Build(perMeshDescriptorSet, perMeshDescriptorSetManager.GetDeviceHandle());
    }

    VkDescriptorSet perMaterialDescriptorSet = perMaterialDescriptorSetManager.GetDescriptorSet(meshMaterial.material.get());
    if (perMaterialDescriptorSet == VK_NULL_HANDLE)
    {
        perMaterialDescriptorSet = perMaterialDescriptorSetManager.AllocateDescriptorSet(meshMaterial.material.get());

        PBRParameterMaterial* pbrMaterial = static_cast<PBRParameterMaterial*>(meshMaterial.material.get());
        VkDescriptorBufferInfo pbrMaterialBufferInfoBufferInfo = pbrMaterial->uniformBuffer.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &pbrMaterialBufferInfoBufferInfo)
            .Build(perMaterialDescriptorSet, perMaterialDescriptorSetManager.GetDeviceHandle());
    }

    VkDescriptorSet discriptorSets[] = { perMeshDescriptorSet , perTransmittancePassDescriptor, perMaterialDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transmittancePipelineLayout, 0, 3, discriptorSets, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshMaterial.indexCount), 1, static_cast<uint32_t>(meshMaterial.startIndex), 0, 0);
}

void MBOITStaticMeshPBRParametersPipeline::CreateAbsorbancePipeline(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass absorbanceRenderPass)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build(device, &absorbanceDescriptorSetLayoutMesh);
    absorbanceDescriptorSetManagerMesh.Create(logicalDevice, absorbanceDescriptorSetLayoutMesh, builder.GetLayoutBindings());

    builder = {};
    builder
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &absorbanceDescriptorSetLayoutMaterial);
    absorbanceDescriptorSetManagerMaterial.Create(logicalDevice, absorbanceDescriptorSetLayoutMaterial, builder.GetLayoutBindings());

    builder = {};
    builder
        .AddBindPoint(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &absorbanceDescriptorSetLayoutClusters);
    absorbanceDescriptorSetManagerClusters.Create(logicalDevice, absorbanceDescriptorSetLayoutClusters, builder.GetLayoutBindings());

    PipelineLayoutBuilder(logicalDevice)
        .AddDescriptorSetLayout(absorbanceDescriptorSetLayoutMesh)
        .AddDescriptorSetLayout(absorbanceDescriptorSetLayoutMaterial)
        .AddDescriptorSetLayout(absorbanceDescriptorSetLayoutClusters)
        .AddPushConstant(sizeof(MBOITTransmittanceClusteredConstants), VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(absorbancePipelineLayout);

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkExtent2D swapChainExtent = swapChain.GetExtent();

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

    VkPipelineColorBlendAttachmentState b0ColorBlendAttachment{};
    b0ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    b0ColorBlendAttachment.blendEnable = VK_TRUE;
    b0ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    b0ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    b0ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    b0ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    b0ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    b0ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
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

    //Pipeline
    GraphicsPipelineBuilder pipelineBuilder(logicalDevice);
    pipelineBuilder
        .SetVertexShader("shaders/Transparency/transparency.vert.spv", "main")
        .SetFragmentShader("shaders/Transparency/MBOIT/AbsorbancePass.frag.spv", "main")
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
        .SetRenderPass(absorbanceRenderPass, 0)
        .SetPipelineLayout(absorbancePipelineLayout)
        .Build(absorbancePipeline, nullptr);
}

void MBOITStaticMeshPBRParametersPipeline::CreateTrasnmitanceClusteredPipeline(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, VkRenderPass transmittanceRenderPass, VkDescriptorSetLayout perTransmittancePassDescriptorSetLayout)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder builder;
    builder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build(device, &perMeshDescriptorSetLayout);

    perMeshDescriptorSetManager.Create(logicalDevice, perMeshDescriptorSetLayout, builder.GetLayoutBindings());

    DescriptorSetLayoutBuilder perMaterialLayoutBuilder;
    perMaterialLayoutBuilder
        .AddBindPoint(10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &perMaterialDescriptorSetLayout);

    perMaterialDescriptorSetManager.Create(logicalDevice, perMaterialDescriptorSetLayout, perMaterialLayoutBuilder.GetLayoutBindings());

    std::vector<VkDescriptorSetLayout> layouts
    {
        perMeshDescriptorSetLayout,
        perTransmittancePassDescriptorSetLayout,
        perMaterialDescriptorSetLayout,
    };

    VkPushConstantRange push_constant_range = {};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(MBOITTransmittanceClusteredConstants);
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &push_constant_range;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &transmittancePipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkExtent2D swapChainExtent = swapChain.GetExtent();

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
    colorBlendAttachment.blendEnable = VK_TRUE;
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

    //Pipeline
    GraphicsPipelineBuilder pipelineBuilder(logicalDevice);
    pipelineBuilder
        .SetVertexShader("shaders/Transparency/transparency.vert.spv", "main")
        .SetFragmentShader("shaders/Transparency/MBOIT/TransmittancePass.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, { 1.0, 1.0, 1.0, 1.0 })
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(transmittanceRenderPass, 0)
        .SetPipelineLayout(transmittancePipelineLayout)
        .Build(transmittanceClusteredPipeline, nullptr);
}

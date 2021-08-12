#include "EnvironmentLightPipeline.h"

#include "EnvironmentMap.h"

#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"
#include "Vertex.h"

#include "DescriptorSetLayoutBuilder.h"

void EnvironmentLightPipeline::Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
    descriptorSetLayoutBuilder
        .AddBindPoint(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &perEniromentMapSetLayout);
    
    environmentMapDescriptorSetManager.Create(logicalDevice, perEniromentMapSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

    DescriptorSetLayoutBuilder aoMapdescriptorSetLayoutBuilder;
    aoMapdescriptorSetLayoutBuilder
        .AddBindPoint(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &ambientOcclusionMapSetLayout);

    ambientOcclusionMapDescriptorSetManager.Create(logicalDevice, ambientOcclusionMapSetLayout, aoMapdescriptorSetLayoutBuilder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(perLightingPassDescriptorSetLayout)
        .AddDescriptorSetLayout(perEniromentMapSetLayout)
        .AddDescriptorSetLayout(ambientOcclusionMapSetLayout)
        .Build(pipelineLayout);


    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

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
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;// VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    GraphicsPipelineBuilder(logicalDevice)
        .SetVertexShader("shaders/Deferred/LightingPass.vert.spv", "main")
        .SetFragmentShader("shaders/Deferred/LightingPassEnvironmentLight.frag.spv", "main")
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
        sampler.maxLod = 6; 
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
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = 1; 
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler.anisotropyEnable = VK_TRUE;
        sampler.maxAnisotropy = 16;

        if (vkCreateSampler(device, &sampler, nullptr, &integrationMapSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
}

void EnvironmentLightPipeline::CleanUp(VkDevice device)
{
    environmentMapDescriptorSetManager.CleanUp();
}

void EnvironmentLightPipeline::Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, EnvironmentMap* environmentMap, VkDescriptorSet perLightingPassDescriptorSet, Image& aoMap, VkSampler pointSampler)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &perLightingPassDescriptorSet, 0, nullptr);

    VkDescriptorSet environmentMapDS = environmentMapDescriptorSetManager.GetDescriptorSet(environmentMap);
    if (environmentMapDS == VK_NULL_HANDLE)
    {
        environmentMapDS = CreateEnvironmentMapDescriptorSet(environmentMap);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &environmentMapDS, 0, nullptr);

    VkDescriptorSet aoMapDS = ambientOcclusionMapDescriptorSetManager.GetDescriptorSet(this);
    if (aoMapDS == VK_NULL_HANDLE)
    {
        aoMapDS = ambientOcclusionMapDescriptorSetManager.AllocateDescriptorSet(this);

        auto aoMapInfo = aoMap.GetImageInfo(pointSampler);
        DescriptorSetBuilder()
            .BindImage(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoMapInfo)
            .Build(aoMapDS, ambientOcclusionMapDescriptorSetManager.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &aoMapDS, 0, nullptr);
    

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

VkDescriptorSet EnvironmentLightPipeline::CreateEnvironmentMapDescriptorSet(EnvironmentMap* environmentMap)
{
    VkDescriptorImageInfo raddianceMapImageInfo = environmentMap->raddianceMap.GetImageInfo(enviromentMapSampler);
    VkDescriptorImageInfo irraddianceMapImageInfo = environmentMap->irraddianceMap.GetImageInfo(enviromentMapSampler);
    VkDescriptorImageInfo integrationMapImageInfo = environmentMap->integrationMap.GetImageInfo(integrationMapSampler);

    VkDescriptorSet descriptorSet = environmentMapDescriptorSetManager.AllocateDescriptorSet(environmentMap);

    DescriptorSetBuilder()
        .BindImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &raddianceMapImageInfo)
        .BindImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &irraddianceMapImageInfo)
        .BindImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &integrationMapImageInfo)
        .Build(descriptorSet, environmentMapDescriptorSetManager.GetDeviceHandle());

    return descriptorSet;
}

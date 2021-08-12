#include "DirectionalLightPipeline.h"

#include "ShaderUtils.h"
#include "Vertex.h"
#include "DirectionalLightComponent.h"
#include "GameObject.h"

#include <array>

void DirectionalLightPipeline::Create(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkDevice device = logicalDevice.GetVkDevice();
       
    {
        std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings{};

        setLayoutBindings[0].binding = 5;
        setLayoutBindings[0].descriptorCount = 1;
        setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = setLayoutBindings.size();
        layoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &perDirectionalLightDescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        directionalLightComponentDescriptorSetManager.Create(logicalDevice, perDirectionalLightDescriptorSetLayout, setLayoutBindings);
    }


    VkShaderModule  vertShaderModule = ShaderUtils::LoadShader(logicalDevice, "shaders/Deferred/LightingPass.vert.spv");
    VkShaderModule  fragShaderModule = ShaderUtils::LoadShader(logicalDevice, "shaders/Deferred/LighitingPassDirectionalLight.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo  shaderStages[] =
    {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

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

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

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
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;// VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    std::vector<VkDescriptorSetLayout> layouts
    {
        perLightingPassDescriptorSetLayout,
        perDirectionalLightDescriptorSetLayout
    };

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void DirectionalLightPipeline::CleanUp(VkDevice device)
{
    directionalLightComponentDescriptorSetManager.CleanUp();
}

void DirectionalLightPipeline::Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, const DirectionalLightComponent* directionalLight, VkDescriptorSet perLightingPassDescriptorSet)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    //Just one light at the moment
        //Bind all the descriptors
            //1) PerLightingPass
            //2) Directional Light
            //3) GBuffer

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &perLightingPassDescriptorSet, 0, nullptr);

    VkDescriptorSet directionalLightDS = directionalLightComponentDescriptorSetManager.GetDescriptorSet(const_cast<DirectionalLightComponent*>(directionalLight));
    if (directionalLightDS == VK_NULL_HANDLE)
    {
        directionalLightDS = CreateDirectionalLightDescriptorSet(directionalLight);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &directionalLightDS, 0, nullptr);

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

VkDescriptorSet DirectionalLightPipeline::CreateDirectionalLightDescriptorSet(const DirectionalLightComponent* directionalLight)
{
    VkDescriptorSet descriptorSet = directionalLightComponentDescriptorSetManager.AllocateDescriptorSet(const_cast<DirectionalLightComponent*>(directionalLight));

    VkDescriptorBufferInfo perObjectBufferInfo = directionalLight->GetUniformBuffer().GetDescriptorInfo();

    DescriptorSetBuilder()
        .BindBuffer(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &perObjectBufferInfo)
        .Build(descriptorSet, directionalLightComponentDescriptorSetManager.GetDeviceHandle());

    return descriptorSet;
}

#include "GraphicsPipelineBuilder.h"

#include "ShaderUtils.h"

GraphicsPipelineBuilder::~GraphicsPipelineBuilder()
{
    for (VkShaderModule shaderModule : shaderModules)
    {
        vkDestroyShaderModule(m_logicalDevice.GetVkDevice(), shaderModule, nullptr);
    }
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexShader(const std::string& filename, const std::string& entryPoint)
{
    return SetShaderCommon(filename, entryPoint, VK_SHADER_STAGE_VERTEX_BIT);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetGeometryShader(const std::string& filename, const std::string& entryPoint)
{
    return SetShaderCommon(filename, entryPoint, VK_SHADER_STAGE_GEOMETRY_BIT);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetFragmentShader(const std::string& filename, const std::string& entryPoint)
{
    return SetShaderCommon(filename, entryPoint, VK_SHADER_STAGE_FRAGMENT_BIT);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexInputState(VkVertexInputBindingDescription& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributes)
{
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributes)
{
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescription.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescription.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestart)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = primitiveRestart;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddViewport(VkViewport viewport)
{
    viewports.push_back(viewport);

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddScissor(VkRect2D scissor)
{
    scissors.push_back(scissor);

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddDynamicState(VkDynamicState dynamicState)
{
    dynamicStates.push_back(dynamicState);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRasterizerState(VkPipelineRasterizationStateCreateInfo desiredRasterizerState)
{
    rasterizerState = desiredRasterizerState;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMultisampleState(VkPipelineMultisampleStateCreateInfo desiredMultisampleState)
{
    multisampleState = desiredMultisampleState;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddColourBlendAttachmentState(VkPipelineColorBlendAttachmentState colourBlendAttachmentState)
{
    colourBlendAttachmentStates.push_back(colourBlendAttachmentState);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetColorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, std::array<float, 4> blendConstants)
{
    colorBlending.logicOpEnable = logicOpEnable;
    colorBlending.logicOp = logicOp;
    colorBlending.blendConstants[0] = blendConstants[0];
    colorBlending.blendConstants[1] = blendConstants[1];
    colorBlending.blendConstants[2] = blendConstants[2];
    colorBlending.blendConstants[3] = blendConstants[3];
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::DisableDepth()
{
    disableDepth = true;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthStencilState(VkPipelineDepthStencilStateCreateInfo desiredDepthStencilState)
{
    depthStencilState = desiredDepthStencilState;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPipelineLayout(VkPipelineLayout desiredPipelineLayout)
{
    pipelineLayout = desiredPipelineLayout;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetRenderPass(VkRenderPass desiredRenderPass, size_t subpass)
{
    renderPass = desiredRenderPass;
    subpassIndex = subpass;

    return *this;
}

void GraphicsPipelineBuilder::Build(VkPipeline& pipeline, VkPipelineCache pipelineCache = VK_NULL_HANDLE)
{
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = static_cast<uint32_t>(viewports.size());
    viewportState.pViewports = viewports.data();
    viewportState.scissorCount = static_cast<uint32_t>(scissors.size());
    viewportState.pScissors = scissors.data();

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = static_cast<uint32_t>(colourBlendAttachmentStates.size());
    colorBlending.pAttachments = colourBlendAttachmentStates.data();

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizerState;
    pipelineInfo.pMultisampleState = &multisampleState;

    if (disableDepth)
    {
        pipelineInfo.pDepthStencilState = VK_NULL_HANDLE;
    }
    else
    {
        pipelineInfo.pDepthStencilState = &depthStencilState;
    }

    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = subpassIndex;

    if (vkCreateGraphicsPipelines(m_logicalDevice.GetVkDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetShaderCommon(const std::string& filename, const std::string& entryPoint, VkShaderStageFlagBits stage)
{
    VkShaderModule shaderModule = ShaderUtils::LoadShader(m_logicalDevice, filename);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = entryPoint.c_str();

    shaderModules.push_back(shaderModule);
    shaderStages.push_back(shaderStageInfo);

    return *this;
}


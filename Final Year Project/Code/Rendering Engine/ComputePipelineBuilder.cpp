#include "ComputePipelineBuilder.h"

#include "ShaderUtils.h"

ComputePipelineBuilder& ComputePipelineBuilder::SetShader(const std::string& filename, const std::string& entryPoint)
{
    shaderModule = ShaderUtils::LoadShader(m_logicalDevice, filename);

    shaderStageInfo = {};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = entryPoint.c_str();

    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetPipelineLayout(VkPipelineLayout desiredPipelineLayout)
{
    pipelineLayout = desiredPipelineLayout;

    return *this;
}

void ComputePipelineBuilder::Build(VkPipeline& pipeline, VkPipelineCache pipelineCache)
{
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateComputePipelines(m_logicalDevice.GetVkDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(m_logicalDevice.GetVkDevice(), shaderModule, nullptr);
}

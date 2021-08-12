#include "PipelineLayoutBuilder.h"

#include <stdexcept>

PipelineLayoutBuilder& PipelineLayoutBuilder::AddDescriptorSetLayout(VkDescriptorSetLayout layout)
{
    layouts.push_back(layout);

    return *this;
}

PipelineLayoutBuilder& PipelineLayoutBuilder::AddPushConstant(size_t size, VkShaderStageFlagBits stage)
{    
    size_t offset = 0;

    if (pushConstantRanges.size() > 0)
    {
        offset = pushConstantRanges[pushConstantRanges.size() - 1].offset + pushConstantRanges[pushConstantRanges.size() - 1].size;
    }

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = offset;
    pushConstantRange.size = size;
    pushConstantRange.stageFlags = stage;

    pushConstantRanges.push_back(pushConstantRange);

    return *this;
}

void PipelineLayoutBuilder::Build(VkPipelineLayout& pipelineLayout)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();

    if (vkCreatePipelineLayout(m_logicalDevice.GetVkDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

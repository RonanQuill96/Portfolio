#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"

class PipelineLayoutBuilder
{
public:
	PipelineLayoutBuilder(const LogicalDevice& logicalDevice) : m_logicalDevice(logicalDevice) {}

	PipelineLayoutBuilder& AddDescriptorSetLayout(VkDescriptorSetLayout layout);
	PipelineLayoutBuilder& AddPushConstant(size_t size, VkShaderStageFlagBits stage);

	void Build(VkPipelineLayout& pipelineLayout);

private:
	std::vector<VkDescriptorSetLayout> layouts;
	std::vector<VkPushConstantRange> pushConstantRanges;

	const LogicalDevice& m_logicalDevice;
};


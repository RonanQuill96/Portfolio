#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"

class ComputePipelineBuilder
{
public:
	ComputePipelineBuilder(const LogicalDevice& logicalDevice) : m_logicalDevice(logicalDevice) {}

	ComputePipelineBuilder& SetShader(const std::string& filename, const std::string& entryPoint);
	ComputePipelineBuilder& SetPipelineLayout(VkPipelineLayout desiredPipelineLayout);

	void Build(VkPipeline& pipeline, VkPipelineCache pipelineCache = VK_NULL_HANDLE);
private:
	VkPipelineLayout pipelineLayout;

	VkShaderModule shaderModule;
	VkPipelineShaderStageCreateInfo shaderStageInfo;

	const LogicalDevice& m_logicalDevice;
};


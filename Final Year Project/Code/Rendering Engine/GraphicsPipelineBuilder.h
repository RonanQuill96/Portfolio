#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"

#include <array>
#include <string>
#include <vector>

class GraphicsPipelineBuilder
{
public:
	GraphicsPipelineBuilder(const LogicalDevice& logicalDevice) : m_logicalDevice(logicalDevice) {}
	~GraphicsPipelineBuilder();

	GraphicsPipelineBuilder& SetVertexShader(const std::string& filename, const std::string& entryPoint);
	GraphicsPipelineBuilder& SetGeometryShader(const std::string& filename, const std::string& entryPoint);
	GraphicsPipelineBuilder& SetFragmentShader(const std::string& filename, const std::string& entryPoint);

	template<size_t ArraySize>
	GraphicsPipelineBuilder& SetVertexInputState(VkVertexInputBindingDescription& vertexInputBindingDescription, const std::array<VkVertexInputAttributeDescription, ArraySize>& vertexInputAttributes)
	{
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();

		return *this;
	}
	GraphicsPipelineBuilder& SetVertexInputState(VkVertexInputBindingDescription& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributes);
	GraphicsPipelineBuilder& SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributes);

	GraphicsPipelineBuilder& SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestart);

	GraphicsPipelineBuilder& AddViewport(VkViewport viewport);
	GraphicsPipelineBuilder& AddScissor(VkRect2D scissor);

	GraphicsPipelineBuilder& AddDynamicState(VkDynamicState dynamicState);

	GraphicsPipelineBuilder& SetRasterizerState(VkPipelineRasterizationStateCreateInfo desiredRasterizerState);

	GraphicsPipelineBuilder& SetMultisampleState(VkPipelineMultisampleStateCreateInfo desiredMultisampleState);

	GraphicsPipelineBuilder& AddColourBlendAttachmentState(VkPipelineColorBlendAttachmentState colourBlendAttachmentState);
	GraphicsPipelineBuilder& SetColorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, std::array<float, 4> blendConstants);

	GraphicsPipelineBuilder& DisableDepth();
	GraphicsPipelineBuilder& SetDepthStencilState(VkPipelineDepthStencilStateCreateInfo desiredDepthStencilState);

	GraphicsPipelineBuilder& SetPipelineLayout(VkPipelineLayout desiredPipelineLayout);

	GraphicsPipelineBuilder& SetRenderPass(VkRenderPass desiredRenderPass, size_t subpass);

	void Build(VkPipeline& pipeline, VkPipelineCache pipelineCache);
private:
	GraphicsPipelineBuilder& SetShaderCommon(const std::string& filename, const std::string& entryPoint, VkShaderStageFlagBits stage);

	std::vector<std::string> entryPointNames;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;

	VkPipelineRasterizationStateCreateInfo rasterizerState{};
	VkPipelineMultisampleStateCreateInfo multisampleState{};

	std::vector<VkPipelineColorBlendAttachmentState> colourBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo colorBlending{};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};

	std::vector<VkDynamicState> dynamicStates;

	VkPipelineLayout pipelineLayout;

	VkRenderPass renderPass;
	size_t subpassIndex;

	bool disableDepth = false;

	const LogicalDevice& m_logicalDevice;
};


#pragma once

#include "VulkanIncludes.h"

#include <vector>
#include <stdexcept>

class DescriptorSetLayoutBuilder
{
public:
	DescriptorSetLayoutBuilder& AddBindPoint(uint32_t bindPoint, VkDescriptorType type, VkShaderStageFlags shaderStage, uint32_t descriptorCount = 1)
	{
		VkDescriptorSetLayoutBinding binding{};

		binding.binding = bindPoint;
		binding.descriptorCount = descriptorCount;
		binding.descriptorType = type;
		binding.stageFlags = shaderStage;

		setLayoutBindings.push_back(binding);

		return *this;
	}

	void Build(VkDevice device, VkDescriptorSetLayout* descriptorSetLayout)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		layoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	std::vector<VkDescriptorSetLayoutBinding>& GetLayoutBindings()
	{
		return setLayoutBindings;
	}

private:
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
};
#pragma once

#include "VulkanIncludes.h"

#include <vector>

class DescriptorSetBuilder
{
public:
	DescriptorSetBuilder() {}

	DescriptorSetBuilder& BindBuffer(uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo* bufferInfo)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pBufferInfo = bufferInfo;

		writeDescriptorSets.push_back(writeDescriptorSet);

		return *this;
	}

	DescriptorSetBuilder& BindImage(uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo* imageInfo)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = imageInfo;

		writeDescriptorSets.push_back(writeDescriptorSet);

		return *this;
	}

	void Build(VkDescriptorSet& descriptorSet, VkDevice device)
	{
		for (VkWriteDescriptorSet& writeDescriptorSet : writeDescriptorSets)
		{
			writeDescriptorSet.dstSet = descriptorSet;
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

private:
	std::vector< VkWriteDescriptorSet > writeDescriptorSets;
};
#pragma once

#include "VulkanIncludes.h"

#include "LogicalDevice.h"

#include <array>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "DescriptorSetBuilder.h"

template<class KeyType, uint32_t MaxPoolSize = 100, bool AllowDynamicPoolCreation = true>
class DescriptorSetManager
{
	struct DescriptorPoolInfo
	{
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		uint32_t descriptorCount = 0;
	};

public:
	template<std::size_t ArraySize>
	void Create(const LogicalDevice& logicalDevice, VkDescriptorSetLayout desiredDescriptorSetLayout, const std::array<VkDescriptorSetLayoutBinding, ArraySize>& descriptorSetLayoutBindings)
	{
		m_device = logicalDevice.GetVkDevice();
		descriptorSetLayout = desiredDescriptorSetLayout;

		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeMap;
		for (const VkDescriptorSetLayoutBinding& binding : descriptorSetLayoutBindings)
		{
			descriptorTypeMap[binding.descriptorType] += binding.descriptorCount;
		}

		poolSizes.reserve(descriptorTypeMap.size());
		for (const auto& descriptorTypeMapEntry : descriptorTypeMap)
		{
			poolSizes.push_back({ descriptorTypeMapEntry.first, descriptorTypeMapEntry.second});
		}
		
		//CreateCulling an initial pool
		CreateDescriptorPool();
	}

	void Create(const LogicalDevice& logicalDevice, VkDescriptorSetLayout desiredDescriptorSetLayout, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings)
	{
		m_device = logicalDevice.GetVkDevice();
		descriptorSetLayout = desiredDescriptorSetLayout;

		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeMap;
		for (const VkDescriptorSetLayoutBinding& binding : descriptorSetLayoutBindings)
		{
			descriptorTypeMap[binding.descriptorType] += binding.descriptorCount;
		}

		poolSizes.reserve(descriptorTypeMap.size());
		for (const auto& descriptorTypeMapEntry : descriptorTypeMap)
		{
			poolSizes.push_back({ descriptorTypeMapEntry.first, descriptorTypeMapEntry.second });
		}

		//CreateCulling an initial pool
		CreateDescriptorPool();
	}

	VkDescriptorSetLayout GetDecsriptorSetLayout() const
	{
		return descriptorSetLayout;
	}

	VkDevice GetDeviceHandle() const
	{
		return m_device;
	}

	void CleanUp()
	{
		for (auto& descriptorPoolInfo : descriptorPools)
		{
			vkDestroyDescriptorPool(m_device, descriptorPoolInfo->descriptorPool, nullptr);
		}
	}

	VkDescriptorSet AllocateDescriptorSet(KeyType key)
	{
		uint32_t descriptorSetCount = 1;

		DescriptorPoolInfo* descriptorPoolInfo = FindAvailableDescriptorPool(descriptorSetCount);
		if (descriptorPoolInfo == nullptr)
		{
			if constexpr (AllowDynamicPoolCreation)
			{
				descriptorPoolInfo = CreateDescriptorPool();
			}
			else
			{
				throw std::runtime_error("No available pools!");
			}
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPoolInfo->descriptorPool;
		allocInfo.descriptorSetCount = descriptorSetCount;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		//Cache it
		if (descriptorSetCache.find(key) != descriptorSetCache.end())
		{
			throw std::runtime_error("DescriptorSetManager key already present!");
		}
		descriptorSetCache[key] = descriptorSet;

		descriptorPoolInfo->descriptorCount += descriptorSetCount;

		return descriptorSet;
	}

	VkDescriptorSet GetDescriptorSet(KeyType key)
	{
		auto location = descriptorSetCache.find(key);

		if (location != descriptorSetCache.end())
		{
			return location->second;
		}
		else
		{
			return VK_NULL_HANDLE;
		}
	}

private:
	DescriptorPoolInfo* FindAvailableDescriptorPool(size_t descriptorSetCount)
	{
		for (const std::unique_ptr<DescriptorPoolInfo>& descriptorPoolInfo : descriptorPools)
		{
			if ((descriptorPoolInfo->descriptorCount + descriptorSetCount) <= MaxPoolSize)
			{
				return descriptorPoolInfo.get();
			}
		}

		//None available
		return nullptr;
	}

	DescriptorPoolInfo* CreateDescriptorPool()
	{
		descriptorPools.emplace_back(std::make_unique<DescriptorPoolInfo>());
		DescriptorPoolInfo* descriptorPoolInfo = descriptorPools[descriptorPools.size() - 1].get();

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MaxPoolSize;

		if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPoolInfo->descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}

		descriptorPoolInfo->descriptorCount = 0;

		return descriptorPoolInfo;
	}

	std::vector<std::unique_ptr<DescriptorPoolInfo>> descriptorPools;
	std::unordered_map<KeyType, VkDescriptorSet> descriptorSetCache;

	VkDevice m_device;
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector< VkDescriptorPoolSize > poolSizes;
};

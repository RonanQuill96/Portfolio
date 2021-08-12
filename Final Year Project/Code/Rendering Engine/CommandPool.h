#pragma once

#include "VulkanIncludes.h"

#include "PhysicalDevice.h"
#include "LogicalDevice.h"

#include <deque>
#include <mutex>

class CommandPool
{
public:
	CommandPool() = default;
	~CommandPool() = default;

	void Create(const LogicalDevice& logicalDevice, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
	void CleanUp(const LogicalDevice& logicalDevice);

	void Reset(const LogicalDevice& logicalDevice);

	VkCommandBuffer GetPrimaryCommandBuffer(const LogicalDevice& logicalDevice, bool begin = true);
	void FreeAndResetPrimaryCommandBuffer(VkCommandBuffer&& commandBuffer);

	VkCommandBuffer GetSecondaryCommandBuffer(const LogicalDevice& logicalDevice, VkCommandBufferInheritanceInfo* inheritanceInfo, bool begin = true);

	VkCommandPool GetCommandPool() const;

private:
	VkCommandPool commandPool;
	std::deque<VkCommandBuffer> availablePrimaryCommandBuffers;
	std::vector<VkCommandBuffer> pendingPrimaryCommandBuffers;

	std::deque<VkCommandBuffer> availableSecondaryCommandBuffers;
	std::vector<VkCommandBuffer> pendingSeccondaryCommandBuffers;
};


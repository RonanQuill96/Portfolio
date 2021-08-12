#pragma once

#include "VulkanIncludes.h"

#include "PhysicalDevice.h"

class LogicalDevice
{
public:
	LogicalDevice() = default;
	~LogicalDevice() = default;

	LogicalDevice(const LogicalDevice&) = delete;
	LogicalDevice(LogicalDevice&&) = delete;
	LogicalDevice& operator = (const LogicalDevice&) = delete;
	LogicalDevice& operator = (LogicalDevice&&) = delete;

	void Create(const PhysicalDevice& physicalDevice);
	void CleanUp();

	VkQueue GetComputeQueue() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetGeneralQueue() const;
	VkQueue GetPresentQueue() const;

	VkResult WaitForFences(uint32_t fenceCount, const VkFence* fences, VkBool32 waitAll, uint64_t timeout) const;
	VkResult ResetFences(uint32_t fenceCount, const VkFence* fences) const;

	VkResult WaitIdle() const;

	VkDevice GetVkDevice() const;
	
private:
	VkQueue GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex) const;

	VkDevice device;

	VkQueue computeQueue;
	VkQueue graphicsQueue;
	VkQueue generalQueue;
	VkQueue presentQueue;
};


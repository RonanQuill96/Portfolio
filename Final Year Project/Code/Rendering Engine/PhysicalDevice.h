#pragma once

#include "VulkanIncludes.h"

class PhysicalDevice
{
public:
	PhysicalDevice() = default;
	~PhysicalDevice() = default;

	PhysicalDevice(const PhysicalDevice&) = delete;
	PhysicalDevice(PhysicalDevice&&) = delete;
	PhysicalDevice& operator = (const PhysicalDevice&) = delete;
	PhysicalDevice& operator = (PhysicalDevice&&) = delete;

	void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

	VkPhysicalDevice GetVkPhysicalDevice() const;

	VkFormat GetOptimalDepthFormat() const;

	uint32_t FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	
	const VkPhysicalDeviceProperties& GetProperties() const;
	const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const;
	const QueueFamilyIndices& GetQueueFamilyIndices() const;

	VkFormatProperties GetPhysicalDeviceFormatProperties(VkFormat format) const;

	SwapChainSupportDetails GetSwapChainSupportDetails(VkSurfaceKHR surface) const;

private:
	int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice  device, VkSurfaceKHR surface) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	VkFormat FindSupportedDepthFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkPhysicalDevice physicalDevice = nullptr;

	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	QueueFamilyIndices queueFamilyIndices;
	
	VkFormat optimalDepthFormat;
};


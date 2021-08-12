#pragma once

#include "VulkanIncludes.h"

#include <vector>

class VulkanInstance
{
public:
	VulkanInstance() = default;
	~VulkanInstance() = default;

	VulkanInstance(const VulkanInstance&) = delete;
	VulkanInstance(VulkanInstance&&) = delete;
	VulkanInstance& operator = (const VulkanInstance&) = delete;
	VulkanInstance& operator = (VulkanInstance&&) = delete;

	void Create();
	void CleanUp();

	VkInstance GetVkInstance();

private:
	void SetupDebugMessenger();

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
};


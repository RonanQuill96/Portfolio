#pragma once

#include "VulkanIncludes.h"

#include <string_view>

namespace VulkanDebug
{
	void Initialise(VkDevice device);

	void SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);

	void BeginMarkerRegion(VkCommandBuffer cmdbuffer, std::string_view  markerName, glm::vec4 color);	
	void InsertMarker(VkCommandBuffer cmdbuffer, std::string_view markerName, glm::vec4 color);
	void EndMarkerRegion(VkCommandBuffer cmdBuffer);	
};


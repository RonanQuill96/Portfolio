#include "VulkanDebug.h"

PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = VK_NULL_HANDLE;
PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = VK_NULL_HANDLE;

void VulkanDebug::Initialise(VkDevice device)
{
	pfnDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT"));
	pfnDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT"));
	pfnCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT"));
	pfnCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT"));
	pfnCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT"));
}

void VulkanDebug::SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name)
{
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name.data();
		pfnDebugMarkerSetObjectName(device, &nameInfo);
	}
}

void VulkanDebug::BeginMarkerRegion(VkCommandBuffer cmdbuffer, std::string_view markerName, glm::vec4 color)
{
	if (pfnCmdDebugMarkerBegin)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = markerName.data();
		pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
	}
}

void VulkanDebug::InsertMarker(VkCommandBuffer cmdbuffer, std::string_view markerName, glm::vec4 color)
{
	if (pfnCmdDebugMarkerInsert)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = markerName.data();
		pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
	}
}

void VulkanDebug::EndMarkerRegion(VkCommandBuffer cmdBuffer)
{
	if (pfnCmdDebugMarkerEnd)
	{
		pfnCmdDebugMarkerEnd(cmdBuffer);
	}
}

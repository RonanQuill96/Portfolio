#pragma once

#include <vulkan/vulkan.h>

#include "GraphicsAPI.h"

class Buffer
{
public:
	VkBuffer GetBuffer() const;
	VmaAllocation GetAllocation() const;
	VkDeviceSize GetSize() const;

	void Cleanup();

protected:
	Buffer() = default;
	
	AllocatedBuffer buffer;
	VkDeviceSize bufferSize;
};


#pragma once

#include "Buffer.h"

template<class DataType>
class StorageBuffer : public Buffer
{
public:
	void Create(const GraphicsAPI& graphicsAPI, size_t count)
	{
		//static_assert(sizeof(DataType) % 16 == 0);
		m_allocator = graphicsAPI.GetAllocator();
		m_device = graphicsAPI.GetLogicalDevice().GetVkDevice();

		bufferSize = sizeof(DataType) * count;

		buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	void Map()
	{
		void* voidMemory = reinterpret_cast<void*>(rawMemory);
		vmaMapMemory(m_allocator, buffer.allocation, &voidMemory);
		rawMemory = reinterpret_cast<DataType*>(voidMemory);
	}

	void Unmap()
	{
		vmaUnmapMemory(m_allocator, buffer.allocation);
		rawMemory = nullptr;
	}

	DataType& operator[](size_t index)
	{
		return rawMemory[index];
	}

	VkDescriptorBufferInfo GetDescriptorInfo() const
	{
		VkDescriptorBufferInfo bufferInfo;

		bufferInfo.buffer = buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = bufferSize;

		return bufferInfo;
	}

private:
	DataType* rawMemory = nullptr;

	VkDevice m_device;
	VmaAllocator m_allocator;
};


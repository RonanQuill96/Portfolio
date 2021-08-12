#pragma once

#include "Buffer.h"

template<class DataType>
class UniformBuffer : public Buffer
{
public:
	void Create(const GraphicsAPI& graphicsAPI)
	{
        static_assert(sizeof(DataType) % 16 == 0, "Datas type is not alligned correctly");

        m_allocator = graphicsAPI.GetAllocator();
        m_device = graphicsAPI.GetLogicalDevice().GetVkDevice();

		bufferSize = sizeof(DataType);

        buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

    size_t GetSize() const
    {
        return sizeof(DataType);
    }

    void Update(DataType data) const
    {
        void* memory;
        vmaMapMemory(m_allocator, buffer.allocation, &memory);
        memcpy(memory, &data, sizeof(DataType));
        vmaUnmapMemory(m_allocator, buffer.allocation);
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const
    {
        VkDescriptorBufferInfo bufferInfo;

        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(DataType);

        return bufferInfo;
    }

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
};


#pragma once

#include "Buffer.h"

#include <vector>

class VertexBuffer : public Buffer
{
public:
	template<typename VertexType>
	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const std::vector<VertexType>& vertices, bool dyamic = false)
	{
        VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();
        auto allocator = graphicsAPI.GetAllocator();

        bufferSize = sizeof(VertexType) * vertices.size();

        AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data;
        vmaMapMemory(allocator, stagingBuffer.allocation, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(allocator, stagingBuffer.allocation);

        if (dyamic)
        {
            buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
        else
        {
            buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        }

        graphicsAPI.CopyBuffer(commandPool, stagingBuffer.buffer, buffer.buffer, bufferSize);
        
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
	}
};


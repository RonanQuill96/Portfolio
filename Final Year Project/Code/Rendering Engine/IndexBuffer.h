#pragma once

#include "Buffer.h"
#include "CommandPool.h"

#include <vector>

class IndexBuffer : public Buffer
{
public:
    template<typename IndexType>
    void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool,const std::vector<IndexType>& indices, bool dynamic = false)
    {
        VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

        bufferSize = sizeof(IndexType) * indices.size();

        AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer.SetContents(indices.data(), bufferSize);

        if (dynamic)
        {
            buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
        else
        {
            buffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        }

        graphicsAPI.CopyBuffer(commandPool, stagingBuffer.buffer, buffer.buffer, bufferSize);

        stagingBuffer.Destroy();
    }
};


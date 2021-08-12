#include "Buffer.h"

#include <stdexcept>

VkBuffer Buffer::GetBuffer() const
{
    return buffer.buffer;
}

VmaAllocation Buffer::GetAllocation() const
{
    return buffer.allocation;
}

VkDeviceSize Buffer::GetSize() const
{
    return bufferSize;
}

void Buffer::Cleanup()
{
    if (buffer.buffer != nullptr && buffer.allocation != nullptr)
    {
        buffer.Destroy();
    }
}

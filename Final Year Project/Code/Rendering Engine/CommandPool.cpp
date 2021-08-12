#include "CommandPool.h"

#include <stdexcept>

void CommandPool::Create(const LogicalDevice& logicalDevice, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(logicalDevice.GetVkDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandPool::CleanUp(const LogicalDevice& logicalDevice)
{
    VkDevice device = logicalDevice.GetVkDevice();
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void CommandPool::Reset(const LogicalDevice& logicalDevice)
{
    vkResetCommandPool(logicalDevice.GetVkDevice(), commandPool, 0);

    //Move pending command buffers into available
    for (VkCommandBuffer& commandBuffer : pendingPrimaryCommandBuffers)
    {
        availablePrimaryCommandBuffers.push_back(commandBuffer);
    }
    pendingPrimaryCommandBuffers.clear();

    for (VkCommandBuffer& commandBuffer : pendingSeccondaryCommandBuffers)
    {
        availableSecondaryCommandBuffers.push_back(commandBuffer);
    }
    pendingSeccondaryCommandBuffers.clear();

}

VkCommandBuffer CommandPool::GetPrimaryCommandBuffer(const LogicalDevice& logicalDevice, bool begin)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    //Try to get one from the queue
    if (!availablePrimaryCommandBuffers.empty())
    {
        commandBuffer = availablePrimaryCommandBuffers.front();
        //vkResetCommandBuffer(commandBuffer, 0);
        //TODO error check?
        availablePrimaryCommandBuffers.pop_front();
    }

    //If not available make a new one
    if (commandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logicalDevice.GetVkDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    pendingPrimaryCommandBuffers.push_back(commandBuffer);

    //Begin Recording
    if (begin)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }
   

    return commandBuffer;
}

/*void CommandPool::FreePrimaryCommandBuffer(VkCommandBuffer&& commandBuffer)
{
    availablePrimaryCommandBuffers.emplace_back(commandBuffer);
    commandBuffer = nullptr;
}*/

void CommandPool::FreeAndResetPrimaryCommandBuffer(VkCommandBuffer&& commandBuffer)
{
    vkResetCommandBuffer(commandBuffer, 0);
    availablePrimaryCommandBuffers.emplace_back(commandBuffer);
    commandBuffer = nullptr;
}

VkCommandBuffer CommandPool::GetSecondaryCommandBuffer(const LogicalDevice& logicalDevice, VkCommandBufferInheritanceInfo* inheritanceInfo, bool begin)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    //Try to get one from the queue
    if (!availableSecondaryCommandBuffers.empty())
    {
        commandBuffer = availableSecondaryCommandBuffers.front();
        //vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        //TODO error check?
        availableSecondaryCommandBuffers.pop_front();
    }

    //If not available make a new one
    if (commandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logicalDevice.GetVkDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    pendingSeccondaryCommandBuffers.push_back(commandBuffer);

    //Begin Recording
    if (begin)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        beginInfo.pInheritanceInfo = inheritanceInfo;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }   

    return commandBuffer;
}

/*void CommandPool::StorePendingSecondaryCommandBuffer(VkCommandBuffer&& commandBuffer)
{
    pendingSeccondaryCommandBuffers.push_back(commandBuffer);
    commandBuffer = nullptr;
}*/

/*void CommandPool::FreeSecondaryCommandBuffer(VkCommandBuffer&& commandBuffer)
{
    std::lock_guard<std::mutex> lockGuard{ secondaryCommandBufferQueueMutex };
    secondaryCommandBuffers.emplace_back(commandBuffer);
    commandBuffer = nullptr;
}*/

VkCommandPool CommandPool::GetCommandPool() const
{
    return commandPool;
}

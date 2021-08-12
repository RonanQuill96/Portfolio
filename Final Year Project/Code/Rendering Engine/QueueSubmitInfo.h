#pragma once

#include "VulkanIncludes.h"

#include <vector>

class QueueSubmitInfo
{
public:
    void AddWait(VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage)
    {
        waitSemaphores.push_back(waitSemaphore);
        waitStages.push_back(waitStage);
    }

    void AddSignal(VkSemaphore signalSemapahore)
    {
        signalSemaphores.push_back(signalSemapahore);
    }

    void AddCommandBuffer(VkCommandBuffer commandBuffer)
    {
        commandBuffers.push_back(commandBuffer);
    }

    VkSubmitInfo GetSubmitInfo() const
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pCommandBuffers = commandBuffers.data();

        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        return submitInfo;
    }

    void SubmitToQueue(VkQueue queue, VkFence fence = nullptr)
    {
        auto submitInfo = GetSubmitInfo();
        if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

private:
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;

    std::vector<VkSemaphore> signalSemaphores;
};
#include "RenderingEngine.h"

#include "ImageUtils.h"
#include "Scene.h"
#include "Vertex.h"

#include "MeshComponent.h"

#include <array>
#include "VulkanDebug.h"
#include "GlobalOptions.h"

void RenderingEngine::Initialise(GLFWwindow* window, size_t windowWidth, size_t windowHeight)
{
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;

    graphicsAPI.Initialise(window, m_windowWidth, m_windowHeight);
    swapChain.Create(graphicsAPI, graphicsAPI.GetSurface(), m_windowWidth, m_windowHeight);

    CreateCommandPool();

    uint32_t graphicsQueueIndex = graphicsAPI.GetPhysicalDevice().GetQueueFamilyIndices().graphicsFamily.value();
    generalCommandPool.Create(graphicsAPI.GetLogicalDevice(), graphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    CreateSyncObjects();

    clusteredDeferredShading.Initialise(graphicsAPI, swapChain, generalCommandPool);

    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    generalCommandPool.Reset(logicalDevice);
}

void RenderingEngine::OnWindowResize(size_t newWidth, size_t newHeight)
{
    framebufferResized = true;
}

const GraphicsAPI& RenderingEngine::GetGraphicsAPI() const
{
    return graphicsAPI;
}

CommandPool& RenderingEngine::GetCommandPool()
{
    return generalCommandPool;
}

void RenderingEngine::RenderFrame(const Scene& scene)
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    logicalDevice.WaitForFences(1, &frameData[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain.GetVkSwapChain(), UINT64_MAX, frameData[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        logicalDevice.WaitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = frameData[currentFrame].inFlightFence;

    //Reset the command buffer for this frame
    commandPools[imageIndex].Reset(graphicsAPI.GetLogicalDevice());

    clusteredDeferredShading.Prepare(graphicsAPI, generalCommandPool, scene);
    clusteredDeferredShading.RenderFrame(graphicsAPI,
        commandPools[imageIndex],
        scene,
        imageIndex,
        currentFrame,
        frameData[currentFrame].imageAvailableSemaphore,
        frameData[currentFrame].renderFinishedSemaphore,
        frameData[currentFrame].inFlightFence, 
        swapChain);
   

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frameData[currentFrame].renderFinishedSemaphore;

    VkSwapchainKHR swapChains[] = { swapChain.GetVkSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(logicalDevice.GetPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    logicalDevice.WaitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderingEngine::WaitForFrame()
{
    graphicsAPI.GetLogicalDevice().WaitIdle();
}

void RenderingEngine::Shutdown()
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    CleanupSwapChain();

    generalCommandPool.CleanUp(graphicsAPI.GetLogicalDevice());

    //vkDestroyDescriptorSetLayout(device, perPbrMaterialDescriptorSetLayout, nullptr);
    //vkDestroyDescriptorSetLayout(device, perObjectDescriptorSetLayout, nullptr);
    //sceneRenderer.CleanUp(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, frameData[i].renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, frameData[i].imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, frameData[i].inFlightFence, nullptr);
    }

    graphicsAPI.CleanUp();
}

void RenderingEngine::CreateCommandPool()
{
    size_t swapChainImageCount = swapChain.GetImageCount();
    commandPools.resize(swapChainImageCount);

    uint32_t graphicsQueueIndex = graphicsAPI.GetPhysicalDevice().GetQueueFamilyIndices().graphicsFamily.value();

    for (size_t index = 0; index < swapChainImageCount; index++)
    {
        commandPools[index].Create(graphicsAPI.GetLogicalDevice(), graphicsQueueIndex, 0);
    }
 }


void RenderingEngine::CreateSyncObjects()
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();
    imagesInFlight.resize(swapChain.GetImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameData[i].renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &frameData[i].inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void RenderingEngine::RecreateSwapChain()
{ 
    graphicsAPI.GetLogicalDevice().WaitIdle();

    CleanupSwapChain();

    swapChain.Create(graphicsAPI, graphicsAPI.GetSurface(), m_windowWidth, m_windowHeight);

    CreateCommandPool();

    //sceneRenderer.OnSwapChainChanged(graphicsAPI, generalCommandPool, swapChain);

    //basicRenderPass.OnSwapChainChangedCreate(graphicsAPI.GetLogicalDevice(), swapChain.GetExtent(), depthFormat, swapChain.GetImageFormat());

    //commandBuffers.resize(swapChainFramebuffers.size());
}

void RenderingEngine::CleanupSwapChain()
{
    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();


    /*for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        //graphicsAPI.GetCommandPool().FreePrimaryCommandBuffer(std::move(primaryCommandBuffers[i]));
        vkFreeCommandBuffers(device, commandPool.GetCommandPool(), 1, &commandBuffers[i]);
    }*/

   // commandPool.Reset(graphicsAPI.GetLogicalDevice());


    for (size_t index = 0; index < commandPools.size(); index++)
    {
        commandPools[index].CleanUp(graphicsAPI.GetLogicalDevice());
    }

    //basicRenderPass.OnSwapChainChangedCleanUp(device);

    swapChain.CleanUp(graphicsAPI.GetLogicalDevice());
}
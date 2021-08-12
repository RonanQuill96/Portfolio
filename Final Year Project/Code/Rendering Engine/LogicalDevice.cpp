#include "LogicalDevice.h"

#include <set>
#include <stdexcept>

void LogicalDevice::Create(const PhysicalDevice& physicalDevice)
{
    const QueueFamilyIndices& indices = physicalDevice.GetQueueFamilyIndices();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies =
    {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
        indices.computeFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.samplerAnisotropy = VK_TRUE;
    enabledFeatures.shaderResourceResidency = VK_TRUE;
    enabledFeatures.shaderResourceMinLod = VK_TRUE;
    enabledFeatures.sparseBinding = VK_TRUE;
    enabledFeatures.sparseResidencyImage2D = VK_TRUE;
    enabledFeatures.geometryShader = VK_TRUE;
    enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &enabledFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

    createInfo.enabledLayerCount = 0; //Deprecated (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDeviceCreateInfo.html)

    if (vkCreateDevice(physicalDevice.GetVkPhysicalDevice(), &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    graphicsQueue = GetQueue(indices.graphicsFamily.value(), 0);
    generalQueue = graphicsQueue;

    presentQueue = GetQueue(indices.presentFamily.value(), 0);
    computeQueue = GetQueue(indices.computeFamily.value(), 0);
}

void LogicalDevice::CleanUp()
{
    vkDestroyDevice(device, nullptr);
}

VkQueue LogicalDevice::GetComputeQueue() const
{
    return computeQueue;
}

VkQueue LogicalDevice::GetGraphicsQueue() const
{
    return graphicsQueue;
}

VkQueue LogicalDevice::GetGeneralQueue() const
{
    return generalQueue;
}

VkQueue LogicalDevice::GetPresentQueue() const
{
    return presentQueue;
}

VkQueue LogicalDevice::GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex) const
{
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
    return queue;
}

VkDevice LogicalDevice::GetVkDevice() const
{
    return device;
}

VkResult LogicalDevice::WaitForFences(uint32_t fenceCount, const VkFence* fences, VkBool32 waitAll, uint64_t timeout) const
{
    return vkWaitForFences(device, fenceCount, fences, waitAll, timeout);
}

VkResult LogicalDevice::ResetFences(uint32_t fenceCount, const VkFence* fences) const
{
    return vkResetFences(device, fenceCount, fences);
}

VkResult LogicalDevice::WaitIdle() const
{
    return vkDeviceWaitIdle(device);
}

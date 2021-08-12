#include "PhysicalDevice.h"

#include <stdexcept>
#include <vector>
#include <map>
#include <set>

void PhysicalDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const VkPhysicalDevice& device : devices)
    {
        int score = RateDeviceSuitability(device, surface);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
    {
        physicalDevice = candidates.rbegin()->second;
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    //Set members
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

    optimalDepthFormat = FindSupportedDepthFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkPhysicalDevice PhysicalDevice::GetVkPhysicalDevice() const
{
    return physicalDevice;
}

VkFormat PhysicalDevice::GetOptimalDepthFormat() const
{
    return optimalDepthFormat;
}

uint32_t PhysicalDevice::FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

const VkPhysicalDeviceProperties& PhysicalDevice::GetProperties() const
{
    return properties;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const
{
    return memoryProperties;
}

const QueueFamilyIndices& PhysicalDevice::GetQueueFamilyIndices() const
{
    return queueFamilyIndices;
}

VkFormatProperties PhysicalDevice::GetPhysicalDeviceFormatProperties(VkFormat format) const
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
    return props;
}

SwapChainSupportDetails PhysicalDevice::GetSwapChainSupportDetails(VkSurfaceKHR surface) const
{
    return QuerySwapChainSupport(physicalDevice, surface);
}

int PhysicalDevice::RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
    {
        return 0;
    }

    if (!CheckDeviceExtensionSupport(device))
    {
        return 0;
    }

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate)
    {
        return 0;
    }

    QueueFamilyIndices indices = FindQueueFamilies(device, surface);
    if (!indices.IsComplete())
    {
        return 0;
    }

    return score;
}

bool PhysicalDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


QueueFamilyIndices PhysicalDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    QueueFamilyIndices indices;

    // Logic to find queue family indices to populate struct with
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = index;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = index;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.computeFamily = index;
        }
        
        if (indices.IsComplete())
        {
            break;
        }

        index++;
    }

    return indices;
}

SwapChainSupportDetails PhysicalDevice::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkFormat PhysicalDevice::FindSupportedDepthFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props = GetPhysicalDeviceFormatProperties(format);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}


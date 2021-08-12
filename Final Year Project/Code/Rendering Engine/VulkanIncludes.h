#pragma once

#include <CodeAnalysis/Warnings.h>
#pragma warning(push)  
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS )

#include <vulkan/vulkan.h>

#pragma warning(pop)

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanDebug.h"

#include <vector>
#include <optional>

// Q: Why is there a __nop() before __debugbreak()?
// A: VS' debug engine has a bug where it will silently swallow explicit
// breakpoint interrupts when single-step debugging either line-by-line or
// over call instructions. This can hide legitimate reasons to trap. Asserts
// for example, which can appear as if the did not fire, leaving a programmer
// unknowingly debugging an undefined process.
#define DebugBreakpoint() (__nop(), __debugbreak())

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    bool IsComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 worldViewMatrix;
    alignas(16) glm::mat4 worldMatrix;
};

const std::vector<const char*> DeviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#pragma once

#include "VulkanInstance.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "CommandPool.h"

struct GLFWwindow;

struct AllocatedBuffer
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocator m_allocator;

	template<class Type>
	void SetContents(Type* conents, size_t size)
	{
		void* data;
		vmaMapMemory(m_allocator, allocation, &data);
		memcpy(data, conents, size);
		vmaUnmapMemory(m_allocator, allocation);
	}

	void Destroy()
	{
		vmaDestroyBuffer(m_allocator, buffer, allocation);
	}
};


struct AllocatedImage
{
	VkImage image;
	VmaAllocation allocation;
	VmaAllocator m_allocator;

	void Destroy()
	{
		vmaDestroyImage(m_allocator, image, allocation);
	}
};

class GraphicsAPI
{
public:
	GraphicsAPI() = default;
	~GraphicsAPI() = default;

	GraphicsAPI(const GraphicsAPI&) = delete;
	GraphicsAPI(GraphicsAPI&&) = delete;
	GraphicsAPI& operator = (const GraphicsAPI&) = delete;
	GraphicsAPI& operator = (GraphicsAPI&&) = delete;

	void Initialise(GLFWwindow* window, size_t width, size_t height);
	void CleanUp();

	AllocatedBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;
	void CopyBuffer(CommandPool& commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

	void CopyBufferToImage(VkCommandBuffer& commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageSubresourceLayers imageSubresource) const;
	void TransitionImageLayout(VkCommandBuffer& commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange) const;

	void CopyBufferToImageImmediate(CommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
	void CopyBufferToImageImmediate(CommandPool& commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageSubresourceLayers imageSubresource) const;

	void TransitionImageLayoutImmediate(CommandPool& commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;
	void TransitionImageLayoutImmediate(CommandPool& commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange) const;

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	
	const VulkanInstance& GetVulkanInstance() const;
	const PhysicalDevice& GetPhysicalDevice() const;
	const LogicalDevice& GetLogicalDevice() const;

	VmaAllocator GetAllocator() const;

	VkSurfaceKHR GetSurface() const;

	VkSampler GetDefaultTextureSampler() const;
	VkSampler GetPointSampler() const;
private:

	void CreateVMA();

	void CreateSurface(GLFWwindow* window);

	void CreateDefaultTextureSampler();
	void CreatePointSampler();

	VulkanInstance vulkanInstance;
	PhysicalDevice physicalDevice;
	LogicalDevice logicalDevice;

	VmaAllocator allocator;

	VkSurfaceKHR surface;

	VkSampler defaultTextureSampler;
	VkSampler pointSampler;
};


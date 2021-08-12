#include "ImageUtils.h"

#include "ImageData.h"

Image ImageUtils::LoadCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapLoadInfo& cubeMapLoadInfo)
{
    Image cubeMap;

    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    //Texture / Cubemap
    ImageData images[6];
    images[0] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.right);
    images[1] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.left);
    images[2] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.top);
    images[3] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.bottom);
    images[4] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.front);
    images[5] = ImageData::LoadImageFromFile(cubeMapLoadInfo.folder + cubeMapLoadInfo.back);

    const VkDeviceSize layerSize = images[0].GetImageSize(); //This is just the size of each layer.
    const VkDeviceSize imageSize = layerSize * 6; //4 since I always load my textures with an alpha channel, and multiply it by 6 because the image must have 6 layers.

    // CreateCulling a host-visible staging buffer that contains the raw image data
    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy over the texture data
    void* data;
    vmaMapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation, &data);

    //Copy the data into the staging buffer.
    for (uint8_t i = 0; i < 6; ++i)
    {
        uint8_t* dest = reinterpret_cast<uint8_t*>(data) + (layerSize * i);

        memcpy(dest, images[i].GetDataPtr(), static_cast<size_t>(layerSize));
    }

    vmaUnmapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation);

    for (uint8_t i = 0; i < 6; ++i)
    {
        images[i].FreeImageData();
    }

    // CreateCulling optimal tiled target image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(images[0].GetWidth()), static_cast<uint32_t>(images[0].GetHeight()), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.arrayLayers = 6;
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &cubeMap.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, cubeMap.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &cubeMap.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, cubeMap.textureImage, cubeMap.textureImageMemory, 0);

    // Setup buffer copy regions for each face including all of its miplevels
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    uint32_t mipLevels = 1;
    bufferCopyRegions.reserve(6 * mipLevels);
    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < mipLevels; level++)
        {
            // Calculate offset into staging buffer for the current mip level and face
            size_t offset = layerSize;

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = images[face].GetWidth() >> level;
            bufferCopyRegion.imageExtent.height = images[face].GetHeight() >> level;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset; //TODO this is probably wrong
            bufferCopyRegions.push_back(bufferCopyRegion);
        }
    }

    // Image barrier for optimal image (target)
    // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;

    VkImageSubresourceLayers imageSubresource{};
    imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.mipLevel = 0;
    imageSubresource.baseArrayLayer = 0;
    imageSubresource.layerCount = 6;

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, cubeMap.textureImage, static_cast<uint32_t>(images[0].GetWidth()), static_cast<uint32_t>(images[0].GetHeight()), imageSubresource);
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    // Clean up staging resources
    stagingBuffer.Destroy();

    // CreateCulling image view
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view.format = VK_FORMAT_R8G8B8A8_SRGB;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    view.subresourceRange.layerCount = 6;
    view.subresourceRange.levelCount = mipLevels;
    view.image = cubeMap.textureImage;

    if (vkCreateImageView(device, &view, nullptr, &cubeMap.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return cubeMap;
}

void ImageUtils::SaveCubeMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const CubeMapSaveInfo& cubeMapSaveInfo, const Image& cubeMap)
{
    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    VkDeviceSize bufferSize = cubeMapSaveInfo.width * cubeMapSaveInfo.height * cubeMapSaveInfo.channels; //This is just the size of each layer.
    //const VkDeviceSize imageSize = layerSize * 6; 

    // CreateCulling a host-visible staging buffer that contains the raw image data
    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = cubeMapSaveInfo.mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 6;
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, cubeMapSaveInfo.imageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    for (size_t faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        for (size_t mipLevel = 0; mipLevel < cubeMapSaveInfo.mipLevels; mipLevel++)
        {
            uint32_t width = static_cast<uint32_t>(cubeMapSaveInfo.width) * std::pow(0.5, mipLevel);
            uint32_t height = static_cast<uint32_t>(cubeMapSaveInfo.height) * std::pow(0.5, mipLevel);

            ImageData image = ImageData::CreateImageOfSize(width, height, cubeMapSaveInfo.channels);

            VkCommandBuffer commandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = mipLevel;
            region.imageSubresource.baseArrayLayer = faceIndex;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };

            vkCmdCopyImageToBuffer(
                commandBuffer,
                cubeMap.textureImage,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                stagingBuffer.buffer,
                1,
                &region
            );

            //EndSingleTimeCommands(commandBuffer);
            VkDevice device = logicalDevice.GetVkDevice();
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(logicalDevice.GetGeneralQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(logicalDevice.GetGeneralQueue());

            commandPool.FreeAndResetPrimaryCommandBuffer(std::move(commandBuffer));

            // Copy over the texture data
            void* bufferData;
            vmaMapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation, &bufferData);
            memcpy(image.GetDataPtr(), bufferData, image.GetImageSize());
            vmaUnmapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation);

            ImageData::SaveImageToFile(cubeMapSaveInfo.folder + "/" + std::to_string(faceIndex) + "_level_" + std::to_string(mipLevel) + ".png", image);

            image.FreeImageData();
        }
    }

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, cubeMapSaveInfo.imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    // Clean up staging resources
    stagingBuffer.Destroy();
}

void ImageUtils::SaveImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const ImageSaveInfo& imageSaveInfo, const Image& image, VkFormat imageFormat)
{
    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    ImageData imageData = ImageData::CreateImageOfSize(imageSaveInfo.width, imageSaveInfo.height, imageSaveInfo.channels);

    // CreateCulling a host-visible staging buffer that contains the raw image data
    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(imageData.GetImageSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, image.textureImage, imageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
 
    VkCommandBuffer commandBuffer = commandPool.GetPrimaryCommandBuffer(logicalDevice);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { static_cast<uint32_t>(imageSaveInfo.width), static_cast<uint32_t>(imageSaveInfo.height), 1 };

    vkCmdCopyImageToBuffer(
        commandBuffer,
        image.textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stagingBuffer.buffer,
        1,
        &region
    );

    //EndSingleTimeCommands(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(logicalDevice.GetGeneralQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(logicalDevice.GetGeneralQueue());

    commandPool.FreeAndResetPrimaryCommandBuffer(std::move(commandBuffer));

    // Copy over the texture data
    void* bufferData;
    vmaMapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation, &bufferData);
    memcpy(imageData.GetDataPtr(), bufferData, imageData.GetImageSize());
    vmaUnmapMemory(stagingBuffer.m_allocator, stagingBuffer.allocation);

    ImageData::SaveImageToFile(imageSaveInfo.file, imageData);

    imageData.FreeImageData();

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, image.textureImage, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    // Clean up staging resources
    stagingBuffer.Destroy();
}

bool ImageUtils::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
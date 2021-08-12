#include "EnvironmentMapGenerator.h"

EnvironmentMap EnvironmentMapGenerator::GenerateEnviromentMap(RenderingEngine& renderingEngine, Image source, const Options& options)
{
    EnvironmentMap environmentMap;

    environmentMap.irraddianceMap = irradianceMapPass.GenerateIrradainceMap(renderingEngine, source.textureImageView, options.irradianceMapDimensions);
    environmentMap.raddianceMap = radianceMapGenerationPass.GenerateRadianceMap(renderingEngine, source.textureImageView, options.radianceMapDimensions, options.randianceMapMipCount);
    environmentMap.integrationMap = integrationMapGeneratorPass.GenerateIntegrationMap(renderingEngine, options.integrationMapDimensions);

	return environmentMap;
}

EnvironmentMap EnvironmentMapGenerator::LoadEnviromentMap(RenderingEngine& renderingEngine, const LoadOptions& loadOptions)
{
    EnvironmentMap environmentMap;

    environmentMap.raddianceMap = LoadCubeMapWithMipLevels(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), loadOptions.radianceMap, loadOptions.randianceMapMipCount);
    environmentMap.irraddianceMap = LoadCubeMapWithMipLevels(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), loadOptions.irradianceMap, 1);
    environmentMap.integrationMap = LoadIntegrationMap(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), loadOptions.integration);
    
    return environmentMap;
}

void EnvironmentMapGenerator::SaveEnvironmentMap(const GraphicsAPI& grpahicsAPI, CommandPool& commandPool, EnvironmentMap& environmentMap, const SaveOptions& saveOptions)
{
    CubeMapSaveInfo saveInfo;
    saveInfo.folder = saveOptions.irradianceMap;
    saveInfo.width = saveOptions.environmentMapOptions.irradianceMapDimensions.width;
    saveInfo.height = saveOptions.environmentMapOptions.irradianceMapDimensions.height;
    saveInfo.mipLevels = 1;
    saveInfo.channels = 4;
    saveInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    ImageUtils::SaveCubeMap(grpahicsAPI, commandPool, saveInfo, environmentMap.irraddianceMap);

    saveInfo.folder = saveOptions.radianceMap;
    saveInfo.width = saveOptions.environmentMapOptions.radianceMapDimensions.width;
    saveInfo.height = saveOptions.environmentMapOptions.radianceMapDimensions.height;
    saveInfo.mipLevels = saveOptions.environmentMapOptions.randianceMapMipCount;
    saveInfo.channels = 4;
    saveInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    ImageUtils::SaveCubeMap(grpahicsAPI, commandPool, saveInfo, environmentMap.raddianceMap);

    ImageSaveInfo imageSaveInfo;
    imageSaveInfo.file = saveOptions.integration;
    imageSaveInfo.width = saveOptions.environmentMapOptions.integrationMapDimensions.width;
    imageSaveInfo.height = saveOptions.environmentMapOptions.integrationMapDimensions.height;
    imageSaveInfo.channels = 4;

    ImageUtils::SaveImage(grpahicsAPI, commandPool, imageSaveInfo, environmentMap.integrationMap, VK_FORMAT_R16G16_SFLOAT);
}

void EnvironmentMapGenerator::CleanUp(VkDevice device)
{
}

Image EnvironmentMapGenerator::LoadCubeMapWithMipLevels(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, std::string_view folder, size_t mipLevels)
{
    Image cubeMap;

    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    std::string filename = std::string(folder) + "/0_level_0.png";

    ImageData imageData = ImageData::LoadImageFromFile(filename);
    imageData.FreeImageData();

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(imageData.GetWidth()), static_cast<uint32_t>(imageData.GetHeight()), 1 };
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

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;
    subresourceRange.baseArrayLayer = 0;
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        for (size_t mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            std::string filename = std::string(folder) + "/" + std::to_string(faceIndex) + "_level_" + std::to_string(mipIndex) + ".png";

            ImageData imageData = ImageData::LoadImageFromFile(filename);

            //Copy to buffer
            AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(imageData.GetImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
            stagingBuffer.SetContents(imageData.GetDataPtr(), imageData.GetImageSize());

            imageData.FreeImageData();

            //Copy buffer to image
            VkImageSubresourceLayers imageSubresource{};
            imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageSubresource.mipLevel = mipIndex;
            imageSubresource.baseArrayLayer = faceIndex;
            imageSubresource.layerCount = 1;

            graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, cubeMap.textureImage, static_cast<uint32_t>(imageData.GetWidth()), static_cast<uint32_t>(imageData.GetHeight()), imageSubresource);

            // Clean up staging resources
            stagingBuffer.Destroy();
        }
    }
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, cubeMap.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

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

Image EnvironmentMapGenerator::LoadIntegrationMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, std::string_view filename)
{
    Image image;

    const PhysicalDevice& physicalDevice = graphicsAPI.GetPhysicalDevice();
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    ImageData imageData = ImageData::LoadImageFromFile(filename);

    //Copy to buffer
    AllocatedBuffer stagingBuffer = graphicsAPI.CreateBuffer(imageData.GetImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.SetContents(imageData.GetDataPtr(), imageData.GetImageSize());
    
    imageData.FreeImageData();

    graphicsAPI.CreateImage(imageData.GetWidth(), imageData.GetHeight(), VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.textureImage, image.textureImageMemory);

    graphicsAPI.TransitionImageLayoutImmediate(commandPool, image.textureImage, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphicsAPI.CopyBufferToImageImmediate(commandPool, stagingBuffer.buffer, image.textureImage, static_cast<uint32_t>(imageData.GetWidth()), static_cast<uint32_t>(imageData.GetHeight()));
    graphicsAPI.TransitionImageLayoutImmediate(commandPool, image.textureImage, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.Destroy();

    image.textureImageView = ImageUtils::CreateImageView(logicalDevice, image.textureImage, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

    return image;
}

#include "ReflectionProbeComponent.h"

#include "GameObject.h"
#include "Scene.h"

void ReflectionProbeComponent::RegisterComponent()
{
    GetOwner().GetScene()->renderingScene.AddReflectionProbeToScene(this);
}

void ReflectionProbeComponent::UnregisterComponent()
{
    GetOwner().GetScene()->renderingScene.RemoveReflectionProbeFromScene(this);
}

void ReflectionProbeComponent::Update()
{
    projection = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
    //P[1][1] *= -1;

    const glm::mat4 pos = glm::translate(glm::mat4(1.0f), -GetOwner().GetWorldPosition()); //TODO: why do i do this?

    views[0] = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    views[0] = glm::rotate(views[0], glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * pos;

    views[1] = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    views[1] = glm::rotate(views[1], glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * pos;

    views[2] = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * pos;
    views[3] = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * pos;
    views[4] = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * pos;
    views[5] = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * pos;

    ReflectionProbeUniformBuffer reflectionProbeUniformBuffer;
    for (size_t index = 0; index < 6; index++)
    {
        transform[index] = projection * views[index];
        reflectionProbeUniformBuffer.InverseViews[index] = glm::inverse(views[index]);
        //reflectionProbeUniformBuffer.InverseViews[index] = glm::inverse(transform[index]);
    }

    reflectionProbeUniformBuffer.camPosition = GetOwner().GetWorldPosition();
    reflectionProbeUniformBuffer.InverseProjection = glm::inverse(projection);
    uniformBuffer.Update(reflectionProbeUniformBuffer);
}

void ReflectionProbeComponent::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, size_t pResolution)
{
    //Create a cube map with desired resolution
    resolution = pResolution;

    reflectionProbeGBuffer.Create(graphicsAPI, commandPool, { uint32_t(resolution), uint32_t(resolution) });
    CreateReflectionProbeImage(graphicsAPI, commandPool,resolution);

    uniformBuffer.Create(graphicsAPI);
}

size_t ReflectionProbeComponent::GetResolution() const
{
    return resolution;
}

const std::array<glm::mat4, 6>& ReflectionProbeComponent::GetTransforms() const
{
    return transform;
}

const std::array<glm::mat4, 6>& ReflectionProbeComponent::GetViews() const
{
    return views;
}

const glm::mat4& ReflectionProbeComponent::GetProjection() const
{
    return projection;
}

float ReflectionProbeComponent::GetNearPlane() const
{
    return nearPlane;
}

float ReflectionProbeComponent::GetFarPlane() const
{
    return farPlane;
}

void ReflectionProbeComponent::CreateReflectionProbeImage(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, size_t pResolution)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = resolution;
    imageInfo.extent.height = resolution;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &reflectionProbe.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, reflectionProbe.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &reflectionProbe.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, reflectionProbe.textureImage, reflectionProbe.textureImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = reflectionProbe.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(device, &viewInfo, nullptr, &reflectionProbe.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void ReflectionProbeGBuffer::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkExtent2D viewportExtent)
{
    extent = viewportExtent;

    positions = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);
    normals = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);
    albedo = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);
    metalicRoughness = CreateLayer(graphicsAPI, commandPool, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, viewportExtent);

    depth = CreateLayer(graphicsAPI, commandPool, graphicsAPI.GetPhysicalDevice().GetOptimalDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, viewportExtent);

    CreateSampler(graphicsAPI);
}

VkAttachmentDescription ReflectionProbeGBuffer::GetPositionsLayerAttachmentDesciption() const
{
    VkAttachmentDescription positionsLayer{};

    positionsLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    positionsLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    positionsLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    positionsLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    positionsLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    positionsLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    positionsLayer.format = normals.format;
    positionsLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return positionsLayer;
}

VkAttachmentDescription ReflectionProbeGBuffer::GetNormalsLayerAttachmentDesciption() const
{
    VkAttachmentDescription normalsLayer{};

    normalsLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    normalsLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalsLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalsLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalsLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalsLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalsLayer.format = normals.format;
    normalsLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return normalsLayer;
}

VkAttachmentDescription ReflectionProbeGBuffer::GetAlbedoLayerAttachmentDesciption() const
{
    VkAttachmentDescription albedoLayer{};

    albedoLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    albedoLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    albedoLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    albedoLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    albedoLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    albedoLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    albedoLayer.format = albedo.format;
    albedoLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return albedoLayer;
}

VkAttachmentDescription ReflectionProbeGBuffer::GetMetalicRougnessLayerAttachmentDesciption() const
{
    VkAttachmentDescription metalicRoughnessLayer{};

    metalicRoughnessLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    metalicRoughnessLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    metalicRoughnessLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    metalicRoughnessLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    metalicRoughnessLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    metalicRoughnessLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    metalicRoughnessLayer.format = metalicRoughness.format;
    metalicRoughnessLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return metalicRoughnessLayer;
}

VkAttachmentDescription ReflectionProbeGBuffer::GetDepthLayerAttachmentDesciption() const
{
    VkAttachmentDescription depthLayer{};

    depthLayer.samples = VK_SAMPLE_COUNT_1_BIT;
    depthLayer.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthLayer.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthLayer.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthLayer.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthLayer.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthLayer.format = depthLayer.format;
    depthLayer.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return depthLayer;
}

VkSampler ReflectionProbeGBuffer::GetSampler() const
{
    return gBufferSampler;
}

ReflectionProbeGBuffer::Layer ReflectionProbeGBuffer::CreateLayer(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, VkFormat format, VkImageUsageFlagBits usage, VkExtent2D viewportExtent)
{
    Layer layer;

    layer.format = format;

    VkImageAspectFlags aspectMask = 0;
    VkImageLayout imageLayout;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/;
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkDevice device = graphicsAPI.GetLogicalDevice().GetVkDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = viewportExtent.width;
    imageInfo.extent.height = viewportExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &layer.image.textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, layer.image.textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = graphicsAPI.GetPhysicalDevice().FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &layer.image.textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, layer.image.textureImage, layer.image.textureImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = layer.image.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(device, &viewInfo, nullptr, &layer.image.textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return layer;
}

void ReflectionProbeGBuffer::CreateSampler(const GraphicsAPI& graphicsAPI)
{
}

#include "ClusteredLightCulling.h"

#include "DescriptorSetLayoutBuilder.h"

#include "ComputePipelineBuilder.h"
#include "GraphicsPipelineBuilder.h"
#include "PipelineLayoutBuilder.h"

#include "ShaderUtils.h"

struct ClusteredLightCullingPushObjects
{
    glm::ivec3 tileCount;
    float pad;
    glm::mat4 view;
};

struct CreateClustersPushObjects
{
    float zNear;
    float zFar;
    uint32_t tileSize;
    float pad;

    glm::mat4 inverseProjection;

    glm::vec2 screenDimensions;
};


struct MarkActiveClustersPushConstants
{
    glm::ivec3 clusterCounts;
    int clusterSizeX;

    float zNear;
    float zFar;
    float scale;
    float bias;

    glm::vec2 resolution;
} pushConstants;

size_t ClusteredLightCulling::cluster_x_count;
size_t ClusteredLightCulling::cluster_y_count;

StorageBuffer<CulledLightsPerCluster> ClusteredLightCulling::culledLightsPerCluster;
StorageBuffer<Cluster> ClusteredLightCulling::clusters;
UniformBuffer<PointLightInfo> ClusteredLightCulling::pointLightInfoBuffer;

void ClusteredLightCulling::Create(const GraphicsAPI& graphicsAPI, VkExtent2D viewportExtent)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkDevice device = logicalDevice.GetVkDevice();

    {
        DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
        descriptorSetLayoutBuilder
            .AddBindPoint(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBindPoint(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .AddBindPoint(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(device, &perPointLightDescriptorSetLayout);

        clusteredPointLightsDescriptorSetManagerCulling.Create(logicalDevice, perPointLightDescriptorSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

        PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
        pipelineLayoutBuilder
            .AddDescriptorSetLayout(perPointLightDescriptorSetLayout)
            .AddPushConstant(sizeof(ClusteredLightCullingPushObjects), VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(cullingPipelineLayout);

        ComputePipelineBuilder computePipelineBuilder(logicalDevice);
        computePipelineBuilder
            .SetShader("shaders/Clustered/Cull.comp.spv", "main")
            .SetPipelineLayout(cullingPipelineLayout)
            .Build(cullingPipeline);
    }

    {
        DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
        descriptorSetLayoutBuilder
            .AddBindPoint(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(device, &createClustersDescriptorSetLayout);

        createClustersDescriptorSetManager.Create(logicalDevice, createClustersDescriptorSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

        PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
        pipelineLayoutBuilder
            .AddDescriptorSetLayout(createClustersDescriptorSetLayout)
            .AddPushConstant(sizeof(CreateClustersPushObjects), VK_SHADER_STAGE_COMPUTE_BIT)
            .Build(createClustersPipelineLayout);

        ComputePipelineBuilder computePipelineBuilder(logicalDevice);
        computePipelineBuilder
            .SetShader("shaders/Clustered/CreateClusters.comp.spv", "main")
            .SetPipelineLayout(createClustersPipelineLayout)
            .Build(createClustersPipeline);
    }
  
    pointLightInfoBuffer.Create(graphicsAPI);

    cluster_x_count = (viewportExtent.width - 1) / cluster_size_x + 1;
    cluster_y_count = (viewportExtent.height - 1) / cluster_size_y + 1;
    cluster_count = cluster_x_count * cluster_y_count * cluster_z_slices;

    clusters.Create(graphicsAPI, cluster_count);

    culledLightsPerCluster.Create(graphicsAPI, cluster_count);

    CreateMarkActiveClustersPipeline(graphicsAPI);
}

void ClusteredLightCulling::CreateClusters(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, VkExtent2D viewportExtent)
{
    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_before;
    {
        VkBufferMemoryBarrier clustersBufferBarrier{};
        clustersBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        clustersBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        clustersBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        clustersBufferBarrier.srcQueueFamilyIndex = 0;
        clustersBufferBarrier.dstQueueFamilyIndex = 0;
        clustersBufferBarrier.buffer = clusters.GetBuffer();
        clustersBufferBarrier.size = clusters.GetSize();
        clustersBufferBarrier.offset = 0;
        clustersBufferBarrier.pNext = nullptr;

        barriers_before = { clustersBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_before.size()), barriers_before.data(),
        0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, createClustersPipeline);

    VkDescriptorSet createClustersDS = createClustersDescriptorSetManager.GetDescriptorSet(this);
    if (createClustersDS == VK_NULL_HANDLE)
    {
        createClustersDS = createClustersDescriptorSetManager.AllocateDescriptorSet(this);

        auto clustersInfo = clusters.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &clustersInfo) //cluster info
            .Build(createClustersDS, clusteredPointLightsDescriptorSetManagerCulling.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, createClustersPipelineLayout, 0, 1, &createClustersDS, 0, nullptr);

    //Push constants
    CreateClustersPushObjects createClustersPushObjects;
    createClustersPushObjects.inverseProjection = glm::inverse(cameraComponent->GetProjection());
    createClustersPushObjects.screenDimensions = { viewportExtent.width, viewportExtent.height };
    createClustersPushObjects.zNear = cameraComponent->GetNearPlane();
    createClustersPushObjects.zFar = cameraComponent->GetFarPlane();

    createClustersPushObjects.tileSize = cluster_size_x;
    vkCmdPushConstants(commandBuffer, createClustersPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CreateClustersPushObjects), &createClustersPushObjects);

    vkCmdDispatch(commandBuffer, cluster_x_count, cluster_y_count, cluster_z_slices);

    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_after;
    {
        VkBufferMemoryBarrier clustersBufferBarrier{};
        clustersBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        clustersBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        clustersBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        clustersBufferBarrier.srcQueueFamilyIndex = 0;
        clustersBufferBarrier.dstQueueFamilyIndex = 0;
        clustersBufferBarrier.buffer = clusters.GetBuffer();
        clustersBufferBarrier.size = clusters.GetSize();
        clustersBufferBarrier.offset = 0;
        clustersBufferBarrier.pNext = nullptr;

        barriers_after = { clustersBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_after.size()), barriers_after.data(),
        0, nullptr);
}

void ClusteredLightCulling::CreatePointLightShader(const GraphicsAPI& graphicsAPI, VkRenderPass renderPass, VkExtent2D viewportExtent, VkDescriptorSetLayout perLightingPassDescriptorSetLayout)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();

    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;

    descriptorSetLayoutBuilder
        .AddBindPoint(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBindPoint(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(device, &perPointLightDescriptorSetLayout);

    clusteredPointLightsDescriptorSetManagerFrag.Create(logicalDevice, perPointLightDescriptorSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(perLightingPassDescriptorSetLayout)
        .AddDescriptorSetLayout(perPointLightDescriptorSetLayout)
        .AddPushConstant(sizeof(PLPushObjects), VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(fragPipelineLayout);

    auto bindingDescription = ScreenVertex::GetBindingDescription();
    auto attributeDescriptions = ScreenVertex::GetAttributeDescriptions();

    VkViewport  viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewportExtent.width;
    viewport.height = (float)viewportExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D  scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = viewportExtent;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;// VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    //Create pipeline
    GraphicsPipelineBuilder(logicalDevice)
        .SetVertexShader("shaders/Deferred/LightingPass.vert.spv", "main")
        .SetFragmentShader("shaders/Clustered/PointLight.frag.spv", "main")
        .SetVertexInputState(bindingDescription, attributeDescriptions)
        .SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
        .AddViewport(viewport)
        .AddScissor(scissor)
        .SetRasterizerState(rasterizer)
        .SetMultisampleState(multisampling)
        .AddColourBlendAttachmentState(colorBlendAttachment)
        .SetColorBlendState(VK_FALSE, VkLogicOp::VK_LOGIC_OP_NO_OP, { 1.0, 1.0, 1.0, 1.0 })
        .SetPipelineLayout(fragPipelineLayout)
        .SetDepthStencilState(depthStencil)
        .SetRenderPass(renderPass, 0)
        .Build(fragPipeline, nullptr);
}

void ClusteredLightCulling::MarkActiveClusters(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, const GBuffer& gBuffer, VkExtent2D viewportExtent)
{
    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_before;
    {
        VkBufferMemoryBarrier clustersBufferBarrier{};
        clustersBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        clustersBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        clustersBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        clustersBufferBarrier.srcQueueFamilyIndex = 0;
        clustersBufferBarrier.dstQueueFamilyIndex = 0;
        clustersBufferBarrier.buffer = clusters.GetBuffer();
        clustersBufferBarrier.size = clusters.GetSize();
        clustersBufferBarrier.offset = 0;
        clustersBufferBarrier.pNext = nullptr;

        barriers_before = { clustersBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_before.size()), barriers_before.data(),
        0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, markActiveClustersPipeline);

    VkDescriptorSet markActiveClustersDescriptorSet = markActiveClustersDescriptorSetManager.GetDescriptorSet(this);
    if (markActiveClustersDescriptorSet == VK_NULL_HANDLE)
    {
        markActiveClustersDescriptorSet = markActiveClustersDescriptorSetManager.AllocateDescriptorSet(this);

        auto clustersInfo = clusters.GetDescriptorInfo();
        auto depthInfo = gBuffer.depth.image.GetImageInfo(gBuffer.gBufferSampler);

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &clustersInfo) //cluster info
            .BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthInfo) //cluster info
            .Build(markActiveClustersDescriptorSet, markActiveClustersDescriptorSetManager.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, markActiveClustersPipelineLayout, 0, 1, &markActiveClustersDescriptorSet, 0, nullptr);

    //Push constants
    MarkActiveClustersPushConstants markActiveClustersPushConstants;
    markActiveClustersPushConstants.clusterCounts = { cluster_x_count, cluster_y_count, cluster_z_slices };
    markActiveClustersPushConstants.clusterSizeX = cluster_size_x;
    markActiveClustersPushConstants.zNear = cameraComponent->GetNearPlane();
    markActiveClustersPushConstants.zFar = cameraComponent->GetFarPlane();
    //Basically reduced a log function into a simple multiplication an addition by pre-calculating these
    markActiveClustersPushConstants.scale = (float)cluster_z_slices / std::log2f(markActiveClustersPushConstants.zFar / markActiveClustersPushConstants.zNear);
    markActiveClustersPushConstants.bias = -((float)cluster_z_slices * std::log2f(markActiveClustersPushConstants.zNear) / std::log2f(markActiveClustersPushConstants.zFar / markActiveClustersPushConstants.zNear));
    markActiveClustersPushConstants.resolution.x = viewportExtent.width;
    markActiveClustersPushConstants.resolution.y = viewportExtent.height;

    vkCmdPushConstants(commandBuffer, markActiveClustersPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(MarkActiveClustersPushConstants), &markActiveClustersPushConstants);

    vkCmdDispatch(commandBuffer, cluster_x_count, cluster_y_count, 1);

    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_after;
    {
        VkBufferMemoryBarrier clustersBufferBarrier{};
        clustersBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        clustersBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        clustersBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        clustersBufferBarrier.srcQueueFamilyIndex = 0;
        clustersBufferBarrier.dstQueueFamilyIndex = 0;
        clustersBufferBarrier.buffer = clusters.GetBuffer();
        clustersBufferBarrier.size = clusters.GetSize();
        clustersBufferBarrier.offset = 0;
        clustersBufferBarrier.pNext = nullptr;

        barriers_after = { clustersBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_after.size()), barriers_after.data(),
        0, nullptr);
}

void ClusteredLightCulling::CullLights(VkCommandBuffer commandBuffer, CameraComponent* cameraComponent, const std::vector<PointLightComponent*>* pointLights, VkExtent2D viewportExtent)
{
    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_before;
    {
        VkBufferMemoryBarrier lightVisibilityBufferBarrier{};
        lightVisibilityBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        lightVisibilityBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        lightVisibilityBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        lightVisibilityBufferBarrier.srcQueueFamilyIndex = 0;
        lightVisibilityBufferBarrier.dstQueueFamilyIndex = 0;
        lightVisibilityBufferBarrier.buffer = culledLightsPerCluster.GetBuffer();
        lightVisibilityBufferBarrier.size = culledLightsPerCluster.GetSize();
        lightVisibilityBufferBarrier.offset = 0;
        lightVisibilityBufferBarrier.pNext = nullptr;

        VkBufferMemoryBarrier pointLightInfoBufferBarrier{};
        pointLightInfoBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        pointLightInfoBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        pointLightInfoBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        pointLightInfoBufferBarrier.srcQueueFamilyIndex = 0;
        pointLightInfoBufferBarrier.dstQueueFamilyIndex = 0;
        pointLightInfoBufferBarrier.buffer = pointLightInfoBuffer.GetBuffer();
        pointLightInfoBufferBarrier.size = pointLightInfoBuffer.GetSize();
        pointLightInfoBufferBarrier.offset = 0;
        pointLightInfoBufferBarrier.pNext = nullptr;

        barriers_before = { pointLightInfoBufferBarrier, lightVisibilityBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_before.size()), barriers_before.data(),
        0, nullptr);

    //Copy over info for Max light count

    PointLightInfo pointLightInfo;
    if (pointLights->size() < MAX_LIGHT_PER_PASS)
    {
        pointLightInfo.count = pointLights->size();
    }
    else
    {
        pointLightInfo.count = MAX_LIGHT_PER_PASS;
    }

    for (size_t index = 0; index < pointLightInfo.count; index++)
    {
        pointLightInfo.pointLights[index].lightColour = (*pointLights)[index]->GetColour();
        pointLightInfo.pointLights[index].lightIntensity = (*pointLights)[index]->GetIntensity();
        pointLightInfo.pointLights[index].position = (*pointLights)[index]->GetOwner().GetWorldPosition();
        pointLightInfo.pointLights[index].range = (*pointLights)[index]->GetRange();
    }

    pointLightInfoBuffer.Update(pointLightInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingPipeline);

    VkDescriptorSet cullingDS = clusteredPointLightsDescriptorSetManagerCulling.GetDescriptorSet(this);
    if (cullingDS == VK_NULL_HANDLE)
    {
        cullingDS = clusteredPointLightsDescriptorSetManagerCulling.AllocateDescriptorSet(this);

        auto clustersInfo = clusters.GetDescriptorInfo();
        auto bufferInfo = pointLightInfoBuffer.GetDescriptorInfo();
        auto storageBufferInfo = culledLightsPerCluster.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &clustersInfo) //cluster info
            .BindBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo) //point light info
            .BindBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &storageBufferInfo) //tile info
            .Build(cullingDS, clusteredPointLightsDescriptorSetManagerCulling.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingPipelineLayout, 0, 1, &cullingDS, 0, nullptr);

    //Push constants
    ClusteredLightCullingPushObjects computePushObjects;
    computePushObjects.tileCount = { cluster_x_count , cluster_y_count , cluster_z_slices };
    computePushObjects.view = cameraComponent->GetView();

    vkCmdPushConstants(commandBuffer, cullingPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusteredLightCullingPushObjects), &computePushObjects);

    vkCmdDispatch(commandBuffer, cluster_x_count, cluster_y_count, cluster_z_slices);

    //Add in barrier to ensure buffer is available
    std::vector<VkBufferMemoryBarrier> barriers_after;
    {
        VkBufferMemoryBarrier lightVisibilityBufferBarrier{};
        lightVisibilityBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        lightVisibilityBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        lightVisibilityBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        lightVisibilityBufferBarrier.srcQueueFamilyIndex = 0;
        lightVisibilityBufferBarrier.dstQueueFamilyIndex = 0;
        lightVisibilityBufferBarrier.buffer = culledLightsPerCluster.GetBuffer();
        lightVisibilityBufferBarrier.size = culledLightsPerCluster.GetSize();
        lightVisibilityBufferBarrier.offset = 0;
        lightVisibilityBufferBarrier.pNext = nullptr;

        VkBufferMemoryBarrier pointLightInfoBufferBarrier{};
        pointLightInfoBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        pointLightInfoBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        pointLightInfoBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        pointLightInfoBufferBarrier.srcQueueFamilyIndex = 0;
        pointLightInfoBufferBarrier.dstQueueFamilyIndex = 0;
        pointLightInfoBufferBarrier.buffer = pointLightInfoBuffer.GetBuffer();
        pointLightInfoBufferBarrier.size = pointLightInfoBuffer.GetSize();
        pointLightInfoBufferBarrier.offset = 0;
        pointLightInfoBufferBarrier.pNext = nullptr;

        barriers_after = { pointLightInfoBufferBarrier, lightVisibilityBufferBarrier };
    }

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers_after.size()), barriers_after.data(),
        0, nullptr);
}

void ClusteredLightCulling::Render(VkCommandBuffer commandBuffer, const VertexBuffer& fullscreenQuadVertices, const IndexBuffer& fullscreenQuadIndices, VkDescriptorSet perLightingPassDescriptorSet, CameraComponent* cameraComponent, VkExtent2D viewportExtent)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fragPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fragPipelineLayout, 0, 1, &perLightingPassDescriptorSet, 0, nullptr);

    VkDescriptorSet cullingDS = clusteredPointLightsDescriptorSetManagerFrag.GetDescriptorSet(this);
    if (cullingDS == VK_NULL_HANDLE)
    {
        cullingDS = clusteredPointLightsDescriptorSetManagerFrag.AllocateDescriptorSet(this);

        auto bufferInfo = pointLightInfoBuffer.GetDescriptorInfo();

        VkDescriptorBufferInfo light_visibility_buffer_info = culledLightsPerCluster.GetDescriptorInfo();

        DescriptorSetBuilder()
            .BindBuffer(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo) //point light info
            .BindBuffer(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &light_visibility_buffer_info) //tile info
            .Build(cullingDS, clusteredPointLightsDescriptorSetManagerFrag.GetDeviceHandle());
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fragPipelineLayout, 1, 1, &cullingDS, 0, nullptr);

    //Push constants
    PLPushObjects computePushObjects;
    computePushObjects.clusterCounts = { cluster_x_count, cluster_y_count, cluster_z_slices };
    computePushObjects.clusterSizeX = cluster_size_x;
    computePushObjects.zNear = cameraComponent->GetNearPlane();
    computePushObjects.zFar = cameraComponent->GetFarPlane();
    computePushObjects.cameraPosition = cameraComponent->GetOwner().GetWorldPosition();
    computePushObjects.inverseViewProjection = glm::inverse(cameraComponent->GetProjection() * cameraComponent->GetView());

    //Basically reduced a log function into a simple multiplication an addition by pre-calculating these
    computePushObjects.scale = (float)cluster_z_slices / std::log2f(computePushObjects.zFar / computePushObjects.zNear);
    computePushObjects.bias = -((float)cluster_z_slices * std::log2f(computePushObjects.zNear) / std::log2f(computePushObjects.zFar / computePushObjects.zNear));

    vkCmdPushConstants(commandBuffer, fragPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PLPushObjects), &computePushObjects);

    VkBuffer vertexBuffers[] = { fullscreenQuadVertices.GetBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, fullscreenQuadIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

void ClusteredLightCulling::CreateMarkActiveClustersPipeline(const GraphicsAPI& graphicsAPI)
{
    const LogicalDevice& logicalDevice = graphicsAPI.GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
    descriptorSetLayoutBuilder
        .AddBindPoint(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        .AddBindPoint(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build(device, &markActiveClustersDescriptorSetLayout);

    markActiveClustersDescriptorSetManager.Create(logicalDevice, markActiveClustersDescriptorSetLayout, descriptorSetLayoutBuilder.GetLayoutBindings());

    PipelineLayoutBuilder pipelineLayoutBuilder(logicalDevice);
    pipelineLayoutBuilder
        .AddDescriptorSetLayout(markActiveClustersDescriptorSetLayout)
        .AddPushConstant(sizeof(MarkActiveClustersPushConstants), VK_SHADER_STAGE_COMPUTE_BIT)
        .Build(markActiveClustersPipelineLayout);

    ComputePipelineBuilder computePipelineBuilder(logicalDevice);
    computePipelineBuilder
        .SetShader("shaders/Clustered/GBufferMarkActiveClusters.comp.spv", "main")
        .SetPipelineLayout(markActiveClustersPipelineLayout)
        .Build(markActiveClustersPipeline);
}

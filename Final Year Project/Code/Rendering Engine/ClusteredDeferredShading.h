#pragma once

#include "VulkanIncludes.h"

#include "CommandPool.h"

#include "GraphicsAPI.h"

#include "Scene.h"

#include "DeferredRenderer.h"
#include "MBOIT.h"
#include "ForwardRenderer.h"
#include "UIRenderer.h"

#include "QueueSubmitInfo.h"

#include "ClusteredLightCulling.h"

#include "DebugLinesPipeline.h"

#include "HorizonBasedAmbientOcclusion.h"

class ClusteredDeferredShading
{
public:
	void Initialise(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool);

	void Prepare(const GraphicsAPI& graphicsAPI, CommandPool& generalCommandPool, const Scene& scene);
	void RenderFrame(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore, VkFence fence, SwapChain& swapChain);

    static DebugLinesPipeline debugLinesPipeline;

private:
    void GenerateGBuffer(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore);
    void GenerateClusters(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore imageAvailableSemaphore);
    void CullLights(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame);

    void RenderAmbientOcclusionMap(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame);
    
    void RenderTransparencyAbsorbance(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame);
    void RenderTransparencyTransmittance(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame);

    void OpaqueLightingPass(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame);

    void CompisiteTransparency(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore renderFinishedSemaphore, VkFence fence);
    
    void RenderUI(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const Scene& scene, size_t imageIndex, size_t currentFrame, VkSemaphore renderFinishedSemaphore, VkFence fence);

    void CreateOpaqueRenderPass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain, CommandPool& commandPool);
    void CreateSwapChainRenderPass(const GraphicsAPI& graphicsAPI, const SwapChain& swapChain);

    void CreateRenderImages(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const SwapChain& swapChain);

    void CreateSyncObjects(const GraphicsAPI& graphicsAPI);

    DeferredRenderer deferredRenderer;
    MBOIT mboit;
    ForwardRenderer forwardRenderer;
    UIRenderer uiRenderer;
    HorizonBasedAmbientOcclusion groundTruthAmbientOcclusion;

    struct FrameData
    {
        VkSemaphore gBufferGenerationSemaphore;
        VkSemaphore createClustersSemaphore;
        VkSemaphore lightCulllingSemaphore;

        VkSemaphore ambientOcclusionGenerationSemaphore;

        VkSemaphore absorbanceSemaphore;
        VkSemaphore transmittanceSemaphore;

        VkSemaphore shadingSemaphore;
        VkSemaphore trasnparencyCompositionSemaphore;
    };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 1;
    std::array< FrameData, MAX_FRAMES_IN_FLIGHT> frameData;

    VkExtent2D swapChainExtent;

    VkRenderPass swapChainRenderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass opaqueRenderPass;
    std::vector<Image> opaqueShadingResults;
    std::vector<VkFramebuffer> opaqueFramebuffers;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImagesMemory;
    std::vector<VkImageView> depthImageViews;
};


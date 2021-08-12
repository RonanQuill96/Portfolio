#pragma once

#include "VulkanIncludes.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <algorithm>
#include <limits>
#include <fstream>

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"
#include "ShaderUtils.h"
#include "SwapChain.h"

#include "MBOIT.h"
#include "ClusteredDeferredShading.h"

struct GLFWwindow;
class Scene;
class Camera;

class RenderingEngine
{
public:
    RenderingEngine() = default;
    ~RenderingEngine() = default;

    RenderingEngine(const RenderingEngine&) = delete;
    RenderingEngine(RenderingEngine&&) = delete;
    RenderingEngine& operator = (const RenderingEngine&) = delete;
    RenderingEngine& operator = (RenderingEngine&&) = delete;

	void Initialise(GLFWwindow* window, size_t windowWidth, size_t windowHeight);
    void Shutdown();

    void RenderFrame(const Scene& scene);
    void WaitForFrame();

    void OnWindowResize(size_t newWidth, size_t newHeight);

    const GraphicsAPI& GetGraphicsAPI() const;

    CommandPool& GetCommandPool();

    ClusteredDeferredShading clusteredDeferredShading;
private:
    void CreateCommandPool();

    void CreateSyncObjects();

    void RecreateSwapChain();
    void CleanupSwapChain();

    GraphicsAPI graphicsAPI;

    SwapChain swapChain;

    struct FrameData
    {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;

        VkFence inFlightFence;
    };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 1;
    std::array< FrameData, MAX_FRAMES_IN_FLIGHT> frameData;

    //Per swap chain image data
    std::vector<VkFence> imagesInFlight;

    std::vector<CommandPool> commandPools;

    CommandPool generalCommandPool;

    size_t currentFrame = 0;

    bool framebufferResized = false;
    size_t m_windowWidth = 0;
    size_t m_windowHeight = 0;
};


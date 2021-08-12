#pragma once

#include "VulkanIncludes.h"

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include "RenderingEngine.h"
#include "AssetManager.h"
#include "Scene.h"

#include "GeometryGenerator.h"
#include "GameObject.h"

#include "IrrandianceMapPass.h"
#include "RadianceMapGenerationPass.h"
#include "IntegrationMapGeneratorPass.h"

#include "EnvironmentMap.h"

#include "PointLightComponent.h"

#include "SystemTimer.h"

#include "GlobalOptions.h"


class VulkanApplication
{
public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    void MainLoop();
    void Cleanup();

    void OnResize();
    void OnKey(int key, int scancode, int action, int mods);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);

private:
    void LoadModels();
    void Update();

    void CalculateFPS();

    RenderingEngine renderingEngine;
    AssetManager assetManager;
    SystemTimer systemTimer;

    SkyBox skyBox;
    EnvironmentMap environmentMap;

    SkyBox skyBoxValley;
    EnvironmentMap environmentMapValley;

    GameObject player;

    GameObject directionalLight;

    enum class SelectedDemo
    {
        PBR,
        TRANSPARENCY,
        RENDERING_SYSTEMS,
        CLUSTERED,
        BISTRO
    };

    SelectedDemo selectedDemo = SelectedDemo::PBR;

    void SetCamera(glm::vec3 positions, glm::quat direction);

    struct PBRDemo
    {
        Scene scene;
        std::shared_ptr<GameObject> pbrDemoObject;

        GameObject pointLight;

        glm::vec4 albedo = { 1.0, 0.761, 0.328, 1.0 };
        float metalness = 1.0;
        float roughness = 0.2;

        glm::vec3 pointLightColour = { 1.0, 1.0, 1.0 };
        float pointLightIntensity = 1000;
        float pointLightRange = 1000;

        std::shared_ptr<PBRParameterMaterial> material;
        PointLightComponent* pointLightComponent;
    } pbrDemo;
    void CreatePBRDemoDemo();
    void CreatePBRDemoDemoUI();

    struct TrasnparencyDemo
    {
        Scene scene;
        GameObject pbrDemoSphere;

        GameObject pbrSecondSphere;
        GameObject pbrThirdSphere;

        GameObject pointLight;

        glm::vec4 albedo = { 1.0, 0.0, 0.0, 0.5f };
        float metalness = 0.5;
        float roughness = 0.5;

        glm::vec3 pointLightColour = {1.0, 1.0, 1.0};
        float pointLightIntensity = 1000;
        float pointLightRange = 1000;

        PBRParameterMaterial* material;
        PointLightComponent* pointLightComponent;

    } trasnparencyDemo;
    void CreateTrasnparencyDemo();
    void CreateTrasnparencyDemoUI();

    struct RenderingSystemsDemo
    {
        Scene scene;
        std::vector<std::shared_ptr<GameObject>> sponza;
        std::vector<std::shared_ptr<GameObject>> lights;
        std::vector<PointLightComponent*> lightComponents;

        float pointLightIntensity = 5;
        float pointLightRange = 50;
        
        size_t MAX_LIGHT_COUNT = 100;

        size_t lightCount = 100;

        GameObject relfectionProbeObject;

        size_t polyCount = 0;
        size_t vertexCount = 0;

    } renderingSystemsDemo;
    void CreateRenderingSystemsDemo();
    void CreateRenderSystemDemoUI();

    struct ClusteredDemo
    {
        Scene scene;
        std::vector<std::shared_ptr<GameObject>> rughol;
        std::vector<std::shared_ptr<GameObject>> lights;

        float pointLightIntensity = 50;
        float pointLightRange = 50;

        size_t lightCount = 10000;

        size_t polyCount = 0;
        size_t vertexCount = 0;

        float lightSpeed = 50.0;
    } clusteredDemo;
    void CreateClusteredDemo();
    void CreateClusteredDemoUI();
    void AnimateDemoLights(float delta);

    struct BistroDemo
    {
        Scene scene;
        std::vector<std::shared_ptr<GameObject>> bistro;

        size_t polyCount = 0;
        size_t vertexCount = 0;
    } bistroDemo;
    void CreateBistroDemo();
    void CreateBistroDemoUI();

    GLFWwindow* window;
    const uint32_t WIDTH = 1600;
    const uint32_t HEIGHT = 900;

    float fps = 0.0f;
};


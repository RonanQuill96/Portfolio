#include "VulkanApplication.h"

#include "CameraComponent.h"

#include "ImageUtils.h"

#include "EnvironmentMapGenerator.h"

#include "DirectionalLightComponent.h"

#include "PointLightComponent.h"

#include "HDRToCubeMapConverter.h"

#include <random>

#include <glm/gtx/quaternion.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "ImageData.h"

std::string FormatWithCommas(std::string value, char thousandSep = ',')
{
    int len = value.length();
    int dlen = 3;

    while (len > dlen)
    {
        value.insert(len - dlen, 1, thousandSep);
        dlen += 4;
        len += 1;
    }
    return value;
}

void VulkanApplication::Run()
{
    ImGui::CreateContext();
    InitWindow();
    InitVulkan();

    MainLoop();
    Cleanup();
}

void VulkanApplication::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Ronan Quill FYP", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
}

void VulkanApplication::InitVulkan()
{
    ImageData::InitialiseLoaders();

    renderingEngine.Initialise(window, WIDTH, HEIGHT);

    assetManager.Initialise(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool());

    ImGui_ImplGlfw_InitForVulkan(window, true);
    LoadModels();  
}

void VulkanApplication::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->OnResize();
}

void VulkanApplication::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->OnKey(key, scancode, action, mods);
}

void VulkanApplication::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->OnMouseMove(xpos, ypos);
}

void VulkanApplication::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->OnMouseButton(button, action, mods);
}

void VulkanApplication::MainLoop()
{
    systemTimer.Start();

    while (!glfwWindowShouldClose(window))
    {
        systemTimer.Update();

        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();

        Update();

        if (selectedDemo == SelectedDemo::PBR)
        {
            CreatePBRDemoDemoUI();
            renderingEngine.RenderFrame(pbrDemo.scene);
        }
        else if (selectedDemo == SelectedDemo::TRANSPARENCY)
        {
            CreateTrasnparencyDemoUI();
            renderingEngine.RenderFrame(trasnparencyDemo.scene);
        }
        else if (selectedDemo == SelectedDemo::RENDERING_SYSTEMS)
        {
            CreateRenderSystemDemoUI();
            renderingEngine.RenderFrame(renderingSystemsDemo.scene);
        }
        else if (selectedDemo == SelectedDemo::BISTRO)
        {
            CreateBistroDemoUI();
            renderingEngine.RenderFrame(bistroDemo.scene);
        }
        else if (selectedDemo == SelectedDemo::CLUSTERED)
        {
            CreateClusteredDemoUI();
            renderingEngine.RenderFrame(clusteredDemo.scene);
        }

        CalculateFPS();
    }

    //wait for the logical device to finish operations before exiting MainLoop and destroying the window
    renderingEngine.WaitForFrame();
}


void VulkanApplication::Cleanup()
{
    VkDevice device = renderingEngine.GetGraphicsAPI().GetLogicalDevice().GetVkDevice();
  
    renderingEngine.Shutdown();
    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApplication::OnResize()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    renderingEngine.OnWindowResize(width, height);
}

void VulkanApplication::OnKey(int key, int scancode, int action, int mods)
{
    float movementSpeed = GlobalOptions::GetInstace().cameraSpeed * systemTimer.GetFrameTimeF();
    
    if (key == GLFW_KEY_1 && selectedDemo == SelectedDemo::PBR)
    {
        SetCamera(glm::vec3(4.27663, 2.08432, - 0.795422), glm::quat(0.74648, glm::vec3(-0.103674, 0.651033, 0.0904182)));
    } 

    if (key == GLFW_KEY_1 && selectedDemo == SelectedDemo::TRANSPARENCY)
    {
        SetCamera(glm::vec3(-0.0105727, 1.2387, 5.79841), glm::quat(0.99707, glm::vec3(-0.0689076, 0.0331167, 0.00228877)));
    }

    if (key == GLFW_KEY_1 && selectedDemo == SelectedDemo::RENDERING_SYSTEMS)
    {
        SetCamera(glm::vec3(2.67256, 12.9196, - 15.6503), glm::quat(0.950132, glm::vec3(-0.0306195, 0.31018, 0.00999607)));
    }

    if (key == GLFW_KEY_1 && selectedDemo == SelectedDemo::BISTRO)
    {
        SetCamera(glm::vec3(7.88053, 2.20092, -3.8991), glm::quat(0.911807, glm::vec3(0.0232549, -0.409826, 0.0104523)));
    }

    if (key == GLFW_KEY_1 && selectedDemo == SelectedDemo::CLUSTERED)
    {
        auto points = renderingSystemsDemo.scene.GetActiveCamera()->GetFrustumPoints();

        std::vector<int> indices =
        {
            //Near face
            0, 1,
            1, 2,
            2, 3,
            3, 0,

            //Far face
            4, 5,
            5, 6,
            6, 7,
            7, 4,

            0, 4,
            1, 5,
            2, 6,
            3, 7
        };

        for (int x = 0; x < indices.size(); x+=2)
        {
            auto p1 = points[indices[x]];
            auto p2 = points[indices[x + 1]];

            auto colour = glm::vec3(0.0f, 1.0f, 0.0f);
            ClusteredDeferredShading::debugLinesPipeline.AddLine({ DebugLineVertex(p1, colour), DebugLineVertex(p2, colour) });
        }
    }    
     

    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ 0, 0, -1 * movementSpeed }, GameObject::TransformationSpace::Local);
    }
    else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ 0, 0, 1 * movementSpeed }, GameObject::TransformationSpace::Local);
    }
    else  if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ -1 * movementSpeed, 0, 0 }, GameObject::TransformationSpace::Local);
    }
    else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ 1 * movementSpeed, 0, 0 }, GameObject::TransformationSpace::Local);
    }
    else  if (key == GLFW_KEY_LEFT_CONTROL && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ 0, -1 * movementSpeed, 0 }, GameObject::TransformationSpace::Local);
    }
    else if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        player.Translate({ 0, 1 * movementSpeed, 0 }, GameObject::TransformationSpace::Local);
    }
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        auto position = player.GetWorldPosition();
        std::cout << position.x << " " << position.y << " " << position.z << "\n";

        auto rotation = player.GetWorldRotationQuaternion();
        std::cout << rotation.x << " " << rotation.y << " " << rotation.z << " " << rotation.w << "\n";

        std::cout << "SetCamera(glm::vec3(" << position.x << ", " << position.y << ", " << position.z << "), glm::quat(" << rotation.w << ", glm::vec3(" << rotation.x << ", " << rotation.y << ", " << rotation.z << ")));" << std::endl;
    }
    else if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        selectedDemo = SelectedDemo::PBR;
        GlobalOptions::GetInstace().environmentLight = true;
        HorizonBasedAmbientOcclusion::enable = false;

        SetCamera(glm::vec3(0.00952882, 2.13479, 4.3585), glm::quat(0.988677, glm::vec3(-0.14993, -0.00622559, -0.00094408)));
    }
    else if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        selectedDemo = SelectedDemo::TRANSPARENCY;
        GlobalOptions::GetInstace().environmentLight = true;
        HorizonBasedAmbientOcclusion::enable = false;

        SetCamera(glm::vec3(-4.02663, 0.793224, 5.89765), glm::quat(0.947375, glm::vec3(-0.0716633, - 0.311112, - 0.0235337)));
    }
    else if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
    {
        selectedDemo = SelectedDemo::RENDERING_SYSTEMS;
        GlobalOptions::GetInstace().environmentLight = true;
        HorizonBasedAmbientOcclusion::enable = true;

        SetCamera(glm::vec3(-90, 15, -1.5), glm::quat(0.697358, glm::vec3(0.0, -0.7, 0.0)));
    }
    else if (key == GLFW_KEY_F4 && action == GLFW_PRESS)
    {
        selectedDemo = SelectedDemo::BISTRO;
        GlobalOptions::GetInstace().environmentLight = true;
        HorizonBasedAmbientOcclusion::enable = true;

        SetCamera(glm::vec3(-0.540131, 2.39079, -1.57968), glm::quat(0.706741, glm::vec3(-0.0548835, -0.703223, -0.0546101)));
    }
    else if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
        selectedDemo = SelectedDemo::CLUSTERED;
        GlobalOptions::GetInstace().environmentLight = false;
        HorizonBasedAmbientOcclusion::enable = false; 

        SetCamera(glm::vec3(293.91, 92.6414, -245.72), glm::quat(0.377181, glm::vec3(-0.0915037, 0.895629, 0.217278)));
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        exit(0);
    }
    else if (key == GLFW_KEY_RIGHT_CONTROL && action == GLFW_PRESS)
    {
        DebugBreakpoint();
    }
}

static double prevX = 0;
static double prevY = 0;

void VulkanApplication::OnMouseMove(double xpos, double ypos)
{
    float cameraMoveSpeed = 0.001f;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
    {
        float inputX = xpos - prevX;
        const float cameraYRotation = cameraMoveSpeed * inputX * -1;
        player.Rotate({ 0.0f, cameraYRotation, 0.0f }, GameObject::TransformationSpace::World);

        float inputY = ypos - prevY;
        const float cameraXRotation = cameraMoveSpeed * inputY * -1;
        player.Rotate({ cameraXRotation, 0.0f, 0.0f }, GameObject::TransformationSpace::Local);
    }

    prevX = xpos;
    prevY = ypos;
}

void VulkanApplication::OnMouseButton(int button, int action, int mods)
{
}

template <typename Type>
Type RandomFrom(const Type min, const Type max)
{
    static std::random_device randomNumberGenerator; //std::random_device is a uniformly-distributed integer random number generator that produces non-deterministic random numbers
    static std::default_random_engine randomNumberEngine(randomNumberGenerator()); //This is a random number engine class that generates pseudo-random numbers. It will use the default one uses on this OS.

    using DistributionType = typename std::conditional<std::is_floating_point<Type>::value, std::uniform_real_distribution<Type>, std::uniform_int_distribution<Type>>::type; //Determines whether to use floating point distribution or integer distribution

    DistributionType uniformDistribution(min, max); // Set up the distribution.

    return static_cast<Type>(uniformDistribution(randomNumberEngine)); //Get the random number.
}

void VulkanApplication::LoadModels()
{
    CameraComponent* camera = player.CreateComponent<CameraComponent>();
    camera->CreateCamera(glm::radians(70.0f), WIDTH / (float)HEIGHT, 0.01f, 10000.0f);
    camera->LookAt(glm::vec3(0.0f, 0.0f, 1.0f));

    SetCamera(glm::vec3(0.00952882, 2.13479, 4.3585), glm::quat(0.988677, glm::vec3(-0.14993, -0.00622559, -0.00094408)));
  
    CubeMapLoadInfo skyBoxLoadInfo;
    skyBoxLoadInfo.folder = "Scenes/Amazon Bistro/HDR Cubemap";
    skyBoxLoadInfo.right = "/0_level_0.png";
    skyBoxLoadInfo.left = "/1_level_0.png";
    skyBoxLoadInfo.top = "/2_level_0.png";
    skyBoxLoadInfo.bottom = "/3_level_0.png";
    skyBoxLoadInfo.front = "/4_level_0.png";
    skyBoxLoadInfo.back = "/5_level_0.png";

    skyBox.Load(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), skyBoxLoadInfo);
      
    EnvironmentMapGenerator evg;

    //HDRToCubeMapConverter hdrToCubeMapConverter;
    //Image hdrCubemap = hdrToCubeMapConverter.ConvertHDRToCubeMap(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), "Scenes/Amazon Bistro/san_giuseppe_bridge_4k.hdr", { 1024, 1024 });

   /*CubeMapSaveInfo saveInfo;
    saveInfo.folder = "Scenes/Amazon Bistro/HDR Cubemap";
    saveInfo.width = 1024;
    saveInfo.height = 1024;
    saveInfo.mipLevels = 1;
    saveInfo.channels = 4;
    ImageUtils::SaveCubeMap(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), saveInfo, hdrCubemap);*/

    /*EnvironmentMapGenerator::Options options;
    options.integrationMapDimensions = { 2048, 2048 };
    options.irradianceMapDimensions = { 2048, 2048 };
    options.radianceMapDimensions = { 2048, 2048 };
    options.randianceMapMipCount = 6;

    environmentMap = evg.GenerateEnviromentMap(renderingEngine, skyBox.image, options);
    
    EnvironmentMapGenerator::SaveOptions saveOptions;
    saveOptions.integration = "Textures/San Fran/integration/integration.png";
    saveOptions.irradianceMap = "Textures/San Fran/irradiance";
    saveOptions.radianceMap = "Textures/San Fran/radiance";
    saveOptions.environmentMapOptions = options;
    evg.SaveEnvironmentMap(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), environmentMap, saveOptions);*/
    

    EnvironmentMapGenerator::LoadOptions loadOptions;
    loadOptions.integration = "Textures/San Fran/integration/integration.png";
    loadOptions.irradianceMap = "Textures/San Fran/irradiance";
    loadOptions.radianceMap = "Textures/San Fran/radiance";
    loadOptions.randianceMapMipCount = 6;
    environmentMap = evg.LoadEnviromentMap(renderingEngine, loadOptions);

    skyBoxLoadInfo.folder = "Textures/Valley";
    skyBoxLoadInfo.right = "/px.png";
    skyBoxLoadInfo.left = "/nx.png";
    skyBoxLoadInfo.top = "/py.png";
    skyBoxLoadInfo.bottom = "/ny.png";
    skyBoxLoadInfo.front = "/pz.png";
    skyBoxLoadInfo.back = "/nz.png";
    skyBoxValley.Load(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool(), skyBoxLoadInfo);

    loadOptions.integration = "Textures/Valley/integration/integration.png";
    loadOptions.irradianceMap = "Textures/Valley/irradiance";
    loadOptions.radianceMap = "Textures/Valley/radiance";
    loadOptions.randianceMapMipCount = 6;
    environmentMapValley = evg.LoadEnviromentMap(renderingEngine, loadOptions);

    renderingEngine.GetCommandPool().Reset(renderingEngine.GetGraphicsAPI().GetLogicalDevice());

    CreatePBRDemoDemo();
    CreateTrasnparencyDemo();
    CreateRenderingSystemsDemo();
    CreateBistroDemo();
    CreateClusteredDemo();
}

void VulkanApplication::Update()
{
    if (selectedDemo == SelectedDemo::PBR)
    {
        pbrDemo.scene.Update(0.0f);
    }
    else if (selectedDemo == SelectedDemo::TRANSPARENCY)
    {
        trasnparencyDemo.scene.Update(0.0f);
    }
    else if (selectedDemo == SelectedDemo::RENDERING_SYSTEMS)
    {
        renderingSystemsDemo.scene.Update(0.0f);
    }
    else if (selectedDemo == SelectedDemo::BISTRO)
    {
        bistroDemo.scene.Update(0.0f);
    }
    else if (selectedDemo == SelectedDemo::CLUSTERED)
    {
        clusteredDemo.scene.Update(0.0f);
        AnimateDemoLights(systemTimer.GetFrameTime());
    }


    // Update imGui
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)WIDTH, (float)HEIGHT);
    io.DeltaTime = systemTimer.GetFrameTime();

}

void VulkanApplication::CalculateFPS()
{
    static float secondCounter = 0.0;

    float frameTime = systemTimer.GetFrameTimeF();
    secondCounter += frameTime;

    if (secondCounter > 1.0f)
    {
        fps = 1.0 / frameTime;
        std::cout << "FPS: " << fps << "\n";
        secondCounter = 0.0f;
    }
}

void VulkanApplication::CreateClusteredDemo()
{
    assetManager.LoadBinarySceneFile("Scenes/rungholt/rungholt.scene", clusteredDemo.rughol, clusteredDemo.vertexCount, clusteredDemo.polyCount);
    for (auto& go : clusteredDemo.rughol)
    {
        clusteredDemo.scene.AddToScene(go.get());
    }

    //Create point lights
    for (size_t index = 0; index < clusteredDemo.lightCount; index++)
    {
        std::shared_ptr<GameObject> randomLight = std::make_shared<GameObject>();
        clusteredDemo.lights.push_back(randomLight);

        randomLight->SetWorldPosition({ RandomFrom<int>(-1000, 1000), RandomFrom<int>(0, 100), RandomFrom<int>(-1000, 1000) });

        glm::vec3 colour = { RandomFrom<float>(0.0f, 1.0f), RandomFrom<float>(0.0f, 1.0f), RandomFrom<float>(0.0f, 1.0f) };

        PointLightComponent* pointLight = randomLight->CreateComponent<PointLightComponent>();
        pointLight->Create(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool());
        pointLight->SetColour(colour);
        pointLight->SetIntensity(clusteredDemo.pointLightIntensity);
        pointLight->SetRange(clusteredDemo.pointLightRange);

        clusteredDemo.scene.AddToScene(randomLight.get());
    }

    clusteredDemo.scene.AddToScene(&player);

    clusteredDemo.scene.SetEnvironmentMap(&environmentMap);
    clusteredDemo.scene.SetSkyBox(&skyBox);

    clusteredDemo.scene.Update(0.0f);
}

void VulkanApplication::CreateClusteredDemoUI()
{
    ImGui::NewFrame();

    ImGui::Begin("Metrics");
    ImGui::LabelText(std::to_string(fps).c_str(), "FPS");

    ImGui::LabelText(FormatWithCommas(std::to_string(clusteredDemo.polyCount)).c_str(), "Triangle Count");
    ImGui::LabelText(FormatWithCommas(std::to_string(clusteredDemo.vertexCount)).c_str(), "Vertex Count");

    ImGui::LabelText(FormatWithCommas(std::to_string(clusteredDemo.lightCount)).c_str(), "Light Count");

    ImGui::InputFloat("Animation Speed", &clusteredDemo.lightSpeed, 0.0f, 10.0f, "%.1f");
    ImGui::InputFloat("Camera Speed", &GlobalOptions::GetInstace().cameraSpeed, 1.0f, 10.0f, "%.1f");

    ImGui::Checkbox("AO Enabled", &HorizonBasedAmbientOcclusion::enable);

    ImGui::Checkbox("Environment Light", &GlobalOptions::GetInstace().environmentLight);
    ImGui::End();

    ImGui::Render();
}

void VulkanApplication::AnimateDemoLights(float delta)
{
    for (auto& light : clusteredDemo.lights)
    {
        if (light->GetWorldPosition().y < 0.0)
        {
            light->SetWorldPosition({ light->GetWorldPosition().x, 100.0, light->GetWorldPosition().z });
        }
        light->Translate({ 0, -clusteredDemo.lightSpeed * delta, 0.0 }, GameObject::TransformationSpace::World);
    }
}

void VulkanApplication::SetCamera(glm::vec3 positions, glm::quat direction)
{
    player.SetWorldPosition(positions);
    player.SetWorldRotation(direction);
}

void VulkanApplication::CreatePBRDemoDemo()
{
    size_t polyCount = 0;
    size_t verteCount = 0;
    std::vector<std::shared_ptr<GameObject>> scene;
    assetManager.LoadBinarySceneFile("Scenes/mitsuba/mitsuba.scene", scene, verteCount, polyCount);

    pbrDemo.pbrDemoObject = scene[1];

    pbrDemo.material = std::make_shared<PBRParameterMaterial>();
    pbrDemo.material->materialRenderMode = MaterialRenderMode::Opaque;
    pbrDemo.material->materialName = "Test";
    pbrDemo.material->parameters.albido = pbrDemo.albedo;
    pbrDemo.material->parameters.metalness = pbrDemo.metalness;
    pbrDemo.material->parameters.roughness = pbrDemo.roughness;
    pbrDemo.material->uniformBuffer.Create(renderingEngine.GetGraphicsAPI());
    pbrDemo.material->uniformBuffer.Update(pbrDemo.material->parameters);

    for (const auto& component : pbrDemo.pbrDemoObject->GetComponents())
    {
        MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(component.get());
        if (meshComponent != nullptr)
        {
            for (size_t index = 0; index < meshComponent->mesh->materials.size(); index++)
            {
                meshComponent->mesh->materials[index].material = pbrDemo.material;
            }  
        }
    }

    pbrDemo.pointLight.SetWorldPosition({ 0, 10.0f, 4.0f });
    pbrDemo.pointLightComponent = pbrDemo.pointLight.CreateComponent<PointLightComponent>();
    pbrDemo.pointLightComponent->Create(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool());
    pbrDemo.pointLightComponent->SetColour({ 1.0, 1.0, 1.0 });
    pbrDemo.pointLightComponent->SetIntensity(1000.0f);
    pbrDemo.pointLightComponent->SetRange(1000.0f);

    pbrDemo.scene.AddToScene(&player);

    pbrDemo.scene.AddToScene(pbrDemo.pbrDemoObject.get());
    pbrDemo.scene.AddToScene(&pbrDemo.pointLight);

    pbrDemo.scene.SetEnvironmentMap(&environmentMap);
    pbrDemo.scene.SetSkyBox(&skyBox);
}

void VulkanApplication::CreatePBRDemoDemoUI()
{
    ImGui::NewFrame();

    bool updateRequire = false;
    ImGui::Begin("PBR Parameters");
    if (ImGui::ColorEdit4("Albedo", &pbrDemo.albedo.r))
    {
        updateRequire = true;
    }

    if (ImGui::SliderFloat("Metalness", &pbrDemo.metalness, 0.0f, 1.0f, "%.3f"))
    {
        updateRequire = true;
    }

    if (ImGui::SliderFloat("Roguness", &pbrDemo.roughness, 0.0f, 1.0f, "%.3f"))
    {
        updateRequire = true;
    }

    const char* items[] = { "Gold", "Silver", "Iron", "Plastic - Glossy", "Plastic - Matte", "Pure White Metal" };
    static int item_current = 0;
    if (ImGui::Combo("Material Presets", &item_current, items, IM_ARRAYSIZE(items)))
    {
        switch (item_current)
        {
        case 0:
        {
            pbrDemo.albedo = {1.0, 0.761, 0.328, 1.0};
            pbrDemo.metalness = 1.0;
            pbrDemo.roughness = 0.2;
            updateRequire = true;
        }
            break;

        case 1:
        {
            pbrDemo.albedo = { 0.973, 0.956, 0.913, 1.0 };
            pbrDemo.metalness = 1.0;
            pbrDemo.roughness = 0.2;
            updateRequire = true;
        }
            break;

        case 2:
        {
            pbrDemo.albedo = { 0.552, 0.571,0.571, 1.0 };
            pbrDemo.metalness = 1.0;
            pbrDemo.roughness = 0.2;
            updateRequire = true;
        }
        break;

        case 3:
        {
            pbrDemo.albedo = { 0.0, 0.300, 1.0, 1.0 };
            pbrDemo.metalness = 0.0;
            pbrDemo.roughness = 0.05;
            updateRequire = true;
        }
        break;

        case 4:
        {
            pbrDemo.albedo = { 0.0, 0.300, 1.0, 1.0 };
            pbrDemo.metalness = 0.0;
            pbrDemo.roughness = 0.4;
            updateRequire = true;
        }
        break;

        case 5:
        {
            pbrDemo.albedo = { 1.0, 1.0, 1.0, 1.0 };
            pbrDemo.metalness = 1.0;
            pbrDemo.roughness = 0.0;
            updateRequire = true;
        }
        break;
        }
    }

    ImGui::InputFloat("Camera Speed", &GlobalOptions::GetInstace().cameraSpeed, 1.0f, 10.0f, "%.1f");

    if (updateRequire)
    {
        pbrDemo.material->parameters.albido = pbrDemo.albedo;
        pbrDemo.material->parameters.metalness = pbrDemo.metalness;
        pbrDemo.material->parameters.roughness = pbrDemo.roughness;
        pbrDemo.material->uniformBuffer.Update(pbrDemo.material->parameters);
    }
    //ImGui::EndFrame();
    ImGui::End();


    bool pointLightUpdateNeeded = false;
    ImGui::Begin("Point Light");

    glm::vec3 position = pbrDemo.pointLightComponent->GetOwner().GetWorldPosition();
    if (ImGui::InputFloat3("Position", &position.x))
    {
        pbrDemo.pointLightComponent->GetOwner().SetWorldPosition(position);
    }

    if (ImGui::ColorEdit3("Colour", &pbrDemo.pointLightColour.r))
    {
        pbrDemo.pointLightComponent->SetColour(pbrDemo.pointLightColour);
        pointLightUpdateNeeded = true;
    }

    if (ImGui::InputFloat("Intensity", &pbrDemo.pointLightIntensity, 1.0f, 10.0f, "%.1f"))
    {
        pbrDemo.pointLightComponent->SetIntensity(pbrDemo.pointLightIntensity);
        pointLightUpdateNeeded = true;
    }

    if (ImGui::InputFloat("Range", &pbrDemo.pointLightRange, 1.0f, 10.0f, "%.1f"))
    {
        pbrDemo.pointLightComponent->SetRange(pbrDemo.pointLightRange);
        pointLightUpdateNeeded = true;
    }

    ImGui::Checkbox("Environment Light", &GlobalOptions::GetInstace().environmentLight);

    if (pointLightUpdateNeeded)
    {
        pbrDemo.pointLightComponent->Update();
    }

    ImGui::End();

    // Render to generate draw buffers
    ImGui::Render();
}

void VulkanApplication::CreateTrasnparencyDemo()
{
    GeometryGenerator::GenerateSphere(renderingEngine, 1.0, 24, 24, trasnparencyDemo.pbrSecondSphere, { 0.1, 1.0, 0.1, 0.50f }, trasnparencyDemo.metalness, trasnparencyDemo.roughness);
    trasnparencyDemo.pbrSecondSphere.Translate({ 0.0, 0.0, -2.5f }, GameObject::TransformationSpace::World);
    for (const auto& component : trasnparencyDemo.pbrSecondSphere.GetComponents())
    {
        MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(component.get());
        if (meshComponent != nullptr)
        {
            meshComponent->mesh->materials[0].material.get()->materialRenderMode = MaterialRenderMode::Transparent;
        }
    }

    GeometryGenerator::GenerateSphere(renderingEngine, 1.0, 24, 24, trasnparencyDemo.pbrThirdSphere, { 0.1, 0.1, 1.0, 0.50f }, trasnparencyDemo.metalness, trasnparencyDemo.roughness);
    trasnparencyDemo.pbrThirdSphere.Translate({ 0.0, 0.0, 2.5f }, GameObject::TransformationSpace::World);
    for (const auto& component : trasnparencyDemo.pbrThirdSphere.GetComponents())
    {
        MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(component.get());
        if (meshComponent != nullptr)
        {
            meshComponent->mesh->materials[0].material.get()->materialRenderMode = MaterialRenderMode::Transparent;
        }
    }

    GeometryGenerator::GenerateSphere(renderingEngine, 1.0, 24, 24, trasnparencyDemo.pbrDemoSphere, trasnparencyDemo.albedo, trasnparencyDemo.metalness, trasnparencyDemo.roughness);
    for (const auto& component : trasnparencyDemo.pbrDemoSphere.GetComponents())
    {
        MeshComponent* meshComponent = dynamic_cast<MeshComponent*>(component.get());
        if (meshComponent != nullptr)
        {
            trasnparencyDemo.material = static_cast<PBRParameterMaterial*>(meshComponent->mesh->materials[0].material.get());
            trasnparencyDemo.material->materialRenderMode = MaterialRenderMode::Transparent;
        }
    }

    trasnparencyDemo.pointLight.SetWorldPosition({ 0, 10.0f, 4.0f });
    trasnparencyDemo.pointLightComponent = trasnparencyDemo.pointLight.CreateComponent<PointLightComponent>();
    trasnparencyDemo.pointLightComponent->Create(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool());
    trasnparencyDemo.pointLightComponent->SetColour({ 1.0, 1.0, 1.0 });
    trasnparencyDemo.pointLightComponent->SetIntensity(1000.0f);
    trasnparencyDemo.pointLightComponent->SetRange(1000.0f);

    trasnparencyDemo.scene.AddToScene(&player);

    trasnparencyDemo.scene.AddToScene(&trasnparencyDemo.pbrDemoSphere);
    trasnparencyDemo.scene.AddToScene(&trasnparencyDemo.pbrSecondSphere);
    trasnparencyDemo.scene.AddToScene(&trasnparencyDemo.pbrThirdSphere);
    trasnparencyDemo.scene.AddToScene(&trasnparencyDemo.pointLight);

    trasnparencyDemo.scene.SetEnvironmentMap(&environmentMap);
    trasnparencyDemo.scene.SetSkyBox(&skyBox);
}

void VulkanApplication::CreateTrasnparencyDemoUI()
{
    ImGui::NewFrame();

    bool updateRequire = false;
    ImGui::Begin("PBR Parameters"); 
    if (ImGui::ColorEdit4("Albedo", &trasnparencyDemo.albedo.r))
    {
        updateRequire = true;
    }

    if (ImGui::SliderFloat("Metalness", &trasnparencyDemo.metalness, 0.0f, 1.0f, "%.3f"))
    {
        updateRequire = true;
    }

    if (ImGui::SliderFloat("Roguness", &trasnparencyDemo.roughness, 0.0f, 1.0f, "%.3f"))
    {
        updateRequire = true;
    }

    ImGui::InputFloat("Camera Speed", &GlobalOptions::GetInstace().cameraSpeed, 1.0f, 10.0f, "%.1f");

    if (updateRequire)
    {
        trasnparencyDemo.material->parameters.albido = trasnparencyDemo.albedo;
        trasnparencyDemo.material->parameters.metalness = trasnparencyDemo.metalness;
        trasnparencyDemo.material->parameters.roughness = trasnparencyDemo.roughness;
        trasnparencyDemo.material->uniformBuffer.Update(trasnparencyDemo.material->parameters);
    }


    ImGui::SliderFloat("Overestimation", &MBOIT::overestimation, 0.0, 1.0);
    ImGui::InputFloat("Bias", &MBOIT::bias, 0, 0, "%.10f");

    ImGui::End();


    bool pointLightUpdateNeeded = false;
    ImGui::Begin("Point Light");

    glm::vec3 position = trasnparencyDemo.pointLightComponent->GetOwner().GetWorldPosition();
    if (ImGui::InputFloat3("Position", &position.x))
    {
        trasnparencyDemo.pointLightComponent->GetOwner().SetWorldPosition(position);
    }

    if (ImGui::ColorEdit3("Colour", &trasnparencyDemo.pointLightColour.r))
    {
        trasnparencyDemo.pointLightComponent->SetColour(trasnparencyDemo.pointLightColour);
        pointLightUpdateNeeded = true;
    }

    if (ImGui::InputFloat("Intensity", &trasnparencyDemo.pointLightIntensity, 1.0f, 10.0f, "%.1f"))
    {
        trasnparencyDemo.pointLightComponent->SetIntensity(trasnparencyDemo.pointLightIntensity);
        pointLightUpdateNeeded = true;
    }

    if (ImGui::InputFloat("Range", &trasnparencyDemo.pointLightRange, 1.0f, 10.0f, "%.1f"))
    {
        trasnparencyDemo.pointLightComponent->SetRange(trasnparencyDemo.pointLightRange);
        pointLightUpdateNeeded = true;
    }

    ImGui::Checkbox("Environment Light", &GlobalOptions::GetInstace().environmentLight);

    if (pointLightUpdateNeeded)
    {
        trasnparencyDemo.pointLightComponent->Update();
    }

    ImGui::End();

    // Render to generate draw buffers
    ImGui::Render();
}

void VulkanApplication::CreateRenderingSystemsDemo()
{
    assetManager.LoadBinarySceneFile("Scenes/Sponza/sponza_binary.scene", renderingSystemsDemo.sponza, renderingSystemsDemo.vertexCount, renderingSystemsDemo.polyCount);
    renderingSystemsDemo.sponza[0]->SetScale({ 0.1, 0.1, 0.1 });

    auto plant = assetManager.GetMaterial("Material__57");
    plant->materialRenderMode = MaterialRenderMode::Transparent;

    auto leaves = assetManager.GetMaterial("leaf");
    leaves->materialRenderMode = MaterialRenderMode::Transparent;

    auto chain = assetManager.GetMaterial("chain");
    chain->materialRenderMode = MaterialRenderMode::Transparent;
        
    for (auto& go : renderingSystemsDemo.sponza)
    {
        renderingSystemsDemo.scene.AddToScene(go.get());
    }

    //Create point lights
    for (size_t index = 0; index < renderingSystemsDemo.MAX_LIGHT_COUNT; index++)
    {
        std::shared_ptr<GameObject> randomLight = std::make_shared<GameObject>();
        renderingSystemsDemo.lights.push_back(randomLight);

        randomLight->SetWorldPosition({ RandomFrom<int>(-100, 100), RandomFrom<int>(0, 1), RandomFrom<int>(-20, 20) });

        glm::vec3 colour = { RandomFrom<float>(0.0f, 1.0f), RandomFrom<float>(0.0f, 1.0f), RandomFrom<float>(0.0f, 1.0f) };

        PointLightComponent* pointLight = randomLight->CreateComponent<PointLightComponent>();
        pointLight->Create(renderingEngine.GetGraphicsAPI(), renderingEngine.GetCommandPool());
        pointLight->SetColour(colour);
        pointLight->SetIntensity(renderingSystemsDemo.pointLightIntensity);
        pointLight->SetRange(renderingSystemsDemo.pointLightRange);

        renderingSystemsDemo.lightComponents.push_back(pointLight);

        renderingSystemsDemo.scene.AddToScene(randomLight.get());
    }

    renderingSystemsDemo.scene.AddToScene(&player);

    renderingSystemsDemo.scene.SetEnvironmentMap(&environmentMap);
    renderingSystemsDemo.scene.SetSkyBox(&skyBox);

    renderingSystemsDemo.scene.Update(0.0f);
 }

void VulkanApplication::CreateRenderSystemDemoUI()
{
    ImGui::NewFrame();

    bool updateRequire = false;

    ImGui::Begin("Metrics"); 
    ImGui::LabelText(std::to_string(fps).c_str(), "FPS");

    ImGui::LabelText(FormatWithCommas(std::to_string(renderingSystemsDemo.polyCount)).c_str(), "Triangle Count");
    ImGui::LabelText(FormatWithCommas(std::to_string(renderingSystemsDemo.vertexCount)).c_str(), "Vertex Count");

    ImGui::LabelText(FormatWithCommas(std::to_string(renderingSystemsDemo.lightCount)).c_str(), "Light Count");

    ImGui::InputFloat("Camera Speed", &GlobalOptions::GetInstace().cameraSpeed, 1.0f, 10.0f, "%.1f");

    ImGui::End();

    bool pointLightUpdateNeeded = false;
    ImGui::Begin("Point Light Parameters");

    if (ImGui::InputFloat("Intensity", &renderingSystemsDemo.pointLightIntensity, 1.0f, 10.0f, "%.1f"))
    {
        for (auto* pl : renderingSystemsDemo.lightComponents)
        {
            pl->SetIntensity(renderingSystemsDemo.pointLightIntensity);
        }

        pointLightUpdateNeeded = true;
    }

    if (ImGui::InputFloat("Range", &renderingSystemsDemo.pointLightRange, 1.0f, 10.0f, "%.1f"))
    {
        for (auto* pl : renderingSystemsDemo.lightComponents)
        {
            pl->SetRange(renderingSystemsDemo.pointLightRange);
        }

        pointLightUpdateNeeded = true;
    }

    ImGui::Checkbox("Environment Light", &GlobalOptions::GetInstace().environmentLight);

    if (pointLightUpdateNeeded)
    {
        for (auto* pl : renderingSystemsDemo.lightComponents)
        {
            pl->Update();
        }
    }

    ImGui::End();


    ImGui::Begin("Ambient Occlusion Parameters");

    ImGui::Checkbox("Enabled", &HorizonBasedAmbientOcclusion::enable);

    ImGui::SliderFloat("Radius", &HorizonBasedAmbientOcclusion::gtaoParameters.R, 0.0, 50.0);
    ImGui::SliderFloat("Strength", &HorizonBasedAmbientOcclusion::gtaoParameters.AOStrength, 0.0, 10.0);
    ImGui::SliderFloat("Max Pixel Radius", &HorizonBasedAmbientOcclusion::gtaoParameters.MaxRadiusPixels, 0.0, 100.0);
    ImGui::SliderInt("Number Directions", &HorizonBasedAmbientOcclusion::gtaoParameters.NumDirections, 0, 50);
    ImGui::SliderInt("Number Samples", &HorizonBasedAmbientOcclusion::gtaoParameters.NumSamples, 0, 20);

    static float angle = 30;
    if (ImGui::SliderFloat("Angle Bias", &angle, 0.0, 180.0))
    {
        HorizonBasedAmbientOcclusion::gtaoParameters.TanBias = std::tan(angle * glm::pi<float>() / 180.0f);
    }

    ImGui::End();

    ImGui::Render();
}

void VulkanApplication::CreateBistroDemo()
{
    assetManager.LoadBinarySceneFile("Scenes/Amazon Bistro/Bistro.scene", bistroDemo.bistro, bistroDemo.vertexCount, bistroDemo.polyCount);

    auto SetTransparent = [&](std::string matName)
    {
        auto mat = assetManager.GetMaterial(matName);
        mat->materialRenderMode = MaterialRenderMode::Transparent;
    };

    SetTransparent("Plants_plants.DoubleSided");  
    SetTransparent("TransparentGlass.DoubleSided");  

    std::shared_ptr<GameObject> dl = std::make_shared<GameObject>();
    DirectionalLightComponent* light = dl->CreateComponent<DirectionalLightComponent>();
    light->SetColour({ 0.734, 0.582766, 0.376542 });
    light->Create(renderingEngine.GetGraphicsAPI());
    light->SetIntensity(1500.0f);

    glm::quat Rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(209.0f)));

    dl->Rotate(Rotation, GameObject::TransformationSpace::World);
    bistroDemo.bistro.push_back(dl);
    for (const auto& go : bistroDemo.bistro)
    {
        bistroDemo.scene.AddToScene(go.get());
    }

    bistroDemo.scene.AddToScene(&player);

    bistroDemo.scene.SetEnvironmentMap(&environmentMap);
    bistroDemo.scene.SetSkyBox(&skyBox);
}

void VulkanApplication::CreateBistroDemoUI()
{
    ImGui::NewFrame();

    ImGui::Begin("Metrics");
    ImGui::LabelText(std::to_string(fps).c_str(), "FPS");

    ImGui::LabelText(FormatWithCommas(std::to_string(bistroDemo.polyCount)).c_str(), "Triangle Count");
    ImGui::LabelText(FormatWithCommas(std::to_string(bistroDemo.vertexCount)).c_str(), "Vertex Count");

    ImGui::InputFloat("Camera Speed", &GlobalOptions::GetInstace().cameraSpeed, 1.0f, 10.0f, "%.1f");

    ImGui::End();

    bool pointLightUpdateNeeded = false;
    ImGui::Begin("Light Parameters");
    ImGui::Checkbox("Environment Light", &GlobalOptions::GetInstace().environmentLight);

    ImGui::SliderFloat("Overestimation", &MBOIT::overestimation, 0.0, 1.0);
    ImGui::End();

    ImGui::Begin("Ambient Occlusion Parameters");

    ImGui::Checkbox("Enabled", &HorizonBasedAmbientOcclusion::enable);

    ImGui::SliderFloat("Radius", &HorizonBasedAmbientOcclusion::gtaoParameters.R, 0.0, 50.0);
    ImGui::SliderFloat("Strength", &HorizonBasedAmbientOcclusion::gtaoParameters.AOStrength, 0.0, 10.0);
    ImGui::SliderFloat("Max Pixel Radius", &HorizonBasedAmbientOcclusion::gtaoParameters.MaxRadiusPixels, 0.0, 100.0);
    ImGui::SliderInt("Number Directions", &HorizonBasedAmbientOcclusion::gtaoParameters.NumDirections, 0, 50);
    ImGui::SliderInt("Number Samples", &HorizonBasedAmbientOcclusion::gtaoParameters.NumSamples, 0, 20);

    static float angle = 30;
    if (ImGui::SliderFloat("Angle Bias", &angle, 0.0, 180.0))
    {
        HorizonBasedAmbientOcclusion::gtaoParameters.TanBias = std::tan(angle * glm::pi<float>() / 180.0f);
    }

    ImGui::End();

    ImGui::Render();
}

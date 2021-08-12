#include "AssetManager.h"

#include "RenderingEngine.h"

#include "ImageData.h"

#include "MeshComponent.h"
#include "PointLightComponent.h"
#include "DirectionalLightComponent.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <filesystem>
#include <fstream>

#include "GlobalOptions.h"

void AssetManager::Initialise(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
    m_graphicsAPI = &graphicsAPI;
    m_commandPool = &commandPool;
}

template<class T>
void BasicBinaryRead(std::ifstream& f, T& t)
{
    f.read(reinterpret_cast<char*>(&t), sizeof(T));

}

enum class LightType : uint32_t
{
    DIRECIONAL_LIGHT = 0,
    POINT_LIGHT = 1,
    SPOT_LIGHT = 2
};

enum ComponentType : uint32_t
{
    MESH = 0,
    LIGHT = 1
};

#define FileVersion 5

void AssetManager::LoadBinarySceneFile(std::string filename, std::vector<std::shared_ptr<GameObject>>& gameObjects, size_t& vertexCount, size_t& polyCount)
{

    auto ReadBinaryString = [](std::ifstream& f) -> std::string
    {
        uint64_t stringSize = 0;

        BasicBinaryRead(f, stringSize);

        std::string s;
        s.resize(stringSize);

        f.read(s.data(), stringSize);

        return s;
    };

    std::ifstream inputFile(filename, std::ios::binary);

    std::string directory = std::filesystem::path(filename).parent_path().string();

    size_t version;
    BasicBinaryRead(inputFile, version);

    if (version != FileVersion)
    {
        throw std::runtime_error(filename + " wrong binary file version!");
    }

    //Load materials
    size_t materialCount;
    BasicBinaryRead(inputFile, materialCount);
    for (size_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
    {
        std::shared_ptr<PBRTextureMaterial> material = std::make_shared<PBRTextureMaterial>();

        material->materialName = ReadBinaryString(inputFile);

        //Textures
        std::string albedoTexture = ReadBinaryString(inputFile);
        material->albedo = *LoadTexture(directory + "/" + albedoTexture, VK_FORMAT_R8G8B8A8_SRGB).get();

        std::string metalnessTexture = ReadBinaryString(inputFile);
        material->metalicRougness = *LoadTexture(directory + "/" + metalnessTexture, VK_FORMAT_R8G8_UNORM, 2).get();

        std::string normalTexture = ReadBinaryString(inputFile);
        material->normals = *LoadTexture(directory + "/" + normalTexture, VK_FORMAT_R8G8B8A8_UNORM).get();

        materials[material->materialName] = material;
    }

    //Load meshes
    uint64_t meshCount;
    BasicBinaryRead(inputFile, meshCount);
    for (size_t meshIndex = 0; meshIndex < meshCount; meshIndex++)
    {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

        //name
        mesh->meshName = ReadBinaryString(inputFile);
        if (mesh->meshName == "Bistro_Research_Interior_paris_building_01_interior_2336")
        {
            //DebugBreakpoint();
        }
                  
        //vertice
        size_t vertexCount;
        BasicBinaryRead(inputFile, vertexCount);

        mesh->vertices.resize(vertexCount);

        inputFile.read(reinterpret_cast<char*>(mesh->vertices.data()), vertexCount * sizeof(Vertex));
     
        //indices
        size_t indexCount;
        BasicBinaryRead(inputFile, indexCount);
        mesh->indices.resize(indexCount);
        inputFile.read(reinterpret_cast<char*>(mesh->indices.data()), indexCount * sizeof(uint32_t));

        //materials
        size_t materialCount;
        BasicBinaryRead(inputFile, materialCount);
        mesh->materials.reserve(materialCount);
        for (size_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
        {
            MeshMaterial meshMaterial;

            std::string materialName = ReadBinaryString(inputFile);
            meshMaterial.material = materials[materialName];

            BasicBinaryRead(inputFile, meshMaterial.startIndex);
            BasicBinaryRead(inputFile, meshMaterial.indexCount);

            mesh->materials.push_back(meshMaterial);
        }

        mesh->vertexBuffer.Create(*m_graphicsAPI, *m_commandPool, mesh->vertices);
        mesh->indexBuffer.Create(*m_graphicsAPI, *m_commandPool, mesh->indices);

        mesh->aabb = AABB::CreateAABB(mesh->vertices);

        meshes[mesh->meshName] = mesh;
       
    }

    //Create game objects
    size_t objectCount;
    BasicBinaryRead(inputFile, objectCount);
    gameObjects.reserve(objectCount);
    for (size_t objectIndex = 0; objectIndex < objectCount; objectIndex++)
    {
        //Load game object 
        std::shared_ptr<GameObject> object = std::make_shared<GameObject>();

        //Parent index
        int32_t parentIndex;
        BasicBinaryRead(inputFile, parentIndex);
        if (parentIndex != -1)
        {
            object->SetParent(gameObjects[parentIndex].get());
        }

        glm::mat4 worldMatrix;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                BasicBinaryRead(inputFile, worldMatrix[i][j]);
            }
        }

        glm::vec3 worldPosition;
        glm::quat worldRotation;
        glm::vec3 worldScale;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

        object->SetWorldPosition(worldPosition);
        object->SetWorldRotation(worldRotation);
        object->SetScale(worldScale);

        size_t componentCount;
        BasicBinaryRead(inputFile, componentCount);
        for (size_t componentIndex = 0; componentIndex < componentCount; componentIndex++)
        {
            ComponentType componentType;
            BasicBinaryRead(inputFile, componentType);

            switch (componentType)
            {
            case MESH:
                {
                    //Create mesh component
                    std::string meshName = ReadBinaryString(inputFile);

                    MeshComponent* meshComponent = object->CreateComponent<MeshComponent>();
                    meshComponent->mesh = meshes[meshName];
                    meshComponent->perObjectBuffer.Create(*m_graphicsAPI);

                    vertexCount += meshComponent->mesh->vertices.size();
                    polyCount += meshComponent->mesh->indices.size() / 3;
                }
                break;

            case LIGHT:
                {
                    LightType lightType;
                    glm::vec3 lightColour;
                    float intentsity;

                    BasicBinaryRead(inputFile, lightType);

                    BasicBinaryRead(inputFile, lightColour.x);
                    BasicBinaryRead(inputFile, lightColour.y);
                    BasicBinaryRead(inputFile, lightColour.z);

                    BasicBinaryRead(inputFile, intentsity);

                    switch (lightType)
                    {
                    case LightType::DIRECIONAL_LIGHT:
                    {  
                        DirectionalLightComponent* directionalLight = object->CreateComponent<DirectionalLightComponent>();
                        directionalLight->SetColour(lightColour);
                        directionalLight->SetIntensity(intentsity);
                        directionalLight->Create(*m_graphicsAPI);
                    }
                        break;

                    case LightType::POINT_LIGHT:
                    {   
                        PointLightComponent* pointLight = object->CreateComponent<PointLightComponent>();
                        pointLight->SetColour(lightColour);
                        pointLight->SetIntensity(intentsity/1000);
                        pointLight->SetRange(2000);
                        pointLight->Create(*m_graphicsAPI, *m_commandPool);
                    }                     
                        break;

                    default:
                        break;
                    }
                }
                break;

            default:
                break;
            }

          
        }

        gameObjects.push_back(object);
    }
}

std::shared_ptr<Texture> AssetManager::LoadTexture(const std::string& filename, VkFormat imageFormat, size_t channelCount)
{
    if (textures.find(filename) != textures.end())
    {
        return textures[filename];
    }

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();

    const LogicalDevice& logicalDevice = m_graphicsAPI->GetLogicalDevice();
    VkDevice device = logicalDevice.GetVkDevice();

    ImageData imageData = ImageData::LoadImageFromFile(filename, channelCount);

    AllocatedBuffer stagingBuffer = m_graphicsAPI->CreateBuffer(imageData.GetImageSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    stagingBuffer.SetContents(imageData.GetDataPtr(), imageData.GetImageSize());

    imageData.FreeImageData();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = imageData.GetWidth();
    imageInfo.extent.height = imageData.GetHeight();
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = imageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    texture->m_allocator = m_graphicsAPI->GetAllocator();
    vmaCreateImage(m_graphicsAPI->GetAllocator(), &imageInfo, &allocInfo, &texture->textureImage, &texture->allocation, nullptr);

    m_graphicsAPI->TransitionImageLayoutImmediate(*m_commandPool, texture->textureImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_graphicsAPI->CopyBufferToImageImmediate(*m_commandPool, stagingBuffer.buffer, texture->textureImage, static_cast<uint32_t>(imageData.GetWidth()), static_cast<uint32_t>(imageData.GetHeight()));
    m_graphicsAPI->TransitionImageLayoutImmediate(*m_commandPool, texture->textureImage, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.Destroy();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &texture->textureImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    textures[filename] = texture;

    return texture;
}

std::shared_ptr<Material> AssetManager::GetMaterial(std::string name) const
{
    return materials.at(name);
}

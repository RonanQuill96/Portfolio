#pragma once

#include <memory>
#include <vector>

#include "GameObject.h"
#include "Mesh.h"

#include <unordered_map>

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	AssetManager(const AssetManager&) = delete;
	AssetManager(AssetManager&&) = delete;
	AssetManager& operator = (const AssetManager&) = delete;
	AssetManager& operator = (AssetManager&&) = delete;

	void Initialise(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);

	void LoadBinarySceneFile(std::string filename, std::vector<std::shared_ptr<GameObject>>& gameObjects, size_t& vertexCount, size_t& polyCount);

	std::shared_ptr<Texture> LoadTexture(const std::string& filename, VkFormat imageFormat, size_t channelCount = 4);

	std::shared_ptr<Material> GetMaterial(std::string name) const;

private:
	std::unordered_map<std::string, std::shared_ptr<Material>> materials;
	std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;

	const GraphicsAPI* m_graphicsAPI;
	CommandPool* m_commandPool;
};


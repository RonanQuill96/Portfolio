#pragma once

#include "GameObject.h"
#include "MeshComponent.h"

#include "GraphicsAPI.h"
#include "CommandPool.h"

class GeometryGenerator
{
public:
	static bool GenerateSphere(class RenderingEngine& renderingEngine, const float radius, const unsigned int sliceCount, const unsigned int stackCount, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode = MaterialRenderMode::Opaque);
	static bool GenerateSphere(const GraphicsAPI& graphicsAPI, CommandPool& commandPool, const float radius, const unsigned int sliceCount, const unsigned int stackCount, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode = MaterialRenderMode::Opaque);

	static bool GenerateCube(class RenderingEngine& renderingEngine, const float width, const float height, const float depth, GameObject& gameObject, glm::vec4 albido, float metalness, float roughness, MaterialRenderMode materialRenderMode = MaterialRenderMode::Opaque);
private:
	GeometryGenerator() = default;

	static void GenerateTangents(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};


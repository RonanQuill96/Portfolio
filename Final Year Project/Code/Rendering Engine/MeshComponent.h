#pragma once

#include "BaseComponent.h"

#include "Mesh.h"
#include "MeshMaterial.h"
#include "UniformBuffer.h"

#include "GameObject.h"

#include <memory>
#include <vector>

class MeshComponent final : public BaseComponent
{
public:
	MeshComponent(GameObject& pOwner) : BaseComponent(pOwner) {}

	void RegisterComponent() final;
	void UnregisterComponent() final;

	void Update() final;

	Mesh& GetMesh();
	const Mesh& GetMesh() const;

	//const std::vector<MeshMaterial>& GetMaterials() const;

	const AABB& GetBoundingVolumeLocalSpace() const;
	const AABB& GetBoundingVolumeWorldSpace() const;

//private:
	friend class GeometryGenerator;
	friend class AssetManager;

	std::shared_ptr<Mesh> mesh;
	UniformBuffer<PerObjectBufferInfo> perObjectBuffer;


	AABB boundingVolumeLocalSpace;
	AABB boundingVolumeWorldSpace;
};


#include "MeshComponent.h"

#include "GameObject.h"
#include "Scene.h"

void MeshComponent::RegisterComponent()
{
    GetOwner().GetScene()->renderingScene.AddMeshToScene(this);
}

void MeshComponent::UnregisterComponent()
{
    GetOwner().GetScene()->renderingScene.RemoveMeshFromScene(this);
}

void MeshComponent::Update()
{
    boundingVolumeLocalSpace = mesh->aabb;

    BoundingVolume::UpdateAABB(boundingVolumeLocalSpace, GetOwner().GetWorldMatrix(), boundingVolumeWorldSpace);

    GetOwner().GetScene()->renderingScene.UpdateMesh(this);
}

Mesh& MeshComponent::GetMesh()
{
    return *mesh;
}

const Mesh& MeshComponent::GetMesh() const
{
    return *mesh;
}

const AABB& MeshComponent::GetBoundingVolumeLocalSpace() const
{
    return boundingVolumeLocalSpace;
}

const AABB& MeshComponent::GetBoundingVolumeWorldSpace() const
{
    return boundingVolumeWorldSpace;
}

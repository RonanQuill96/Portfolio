#include "RenderingScene.h"

#include "MeshComponent.h"
#include "DirectionalLightComponent.h"

void RenderingScene::AddMeshToScene(MeshComponent* meshComponent)
{
	meshComponents.push_back(meshComponent);
    meshOctree.Insert(meshComponent);
}

void RenderingScene::RemoveMeshFromScene(MeshComponent* meshComponent)
{
    auto location = std::find(meshComponents.begin(), meshComponents.end(), meshComponent);
    if (location != meshComponents.end())
    {
        meshComponents.erase(location);
    }

    meshOctree.Remove(meshComponent);
}

void RenderingScene::UpdateMesh(MeshComponent* meshComponent)
{
    meshOctree.AddToUpdateSet(meshComponent);
}

void RenderingScene::AddReflectionProbeToScene(ReflectionProbeComponent* reflectionProbeComponent)
{
    reflectionProbes.push_back(reflectionProbeComponent);
}

void RenderingScene::RemoveReflectionProbeFromScene(ReflectionProbeComponent* reflectionProbeComponent)
{
    auto location = std::find(reflectionProbes.begin(), reflectionProbes.end(), reflectionProbeComponent);
    if (location != reflectionProbes.end())
    {
        reflectionProbes.erase(location);
    }
}

void RenderingScene::AddDirectionalLightToScene(DirectionalLightComponent* directionalLight)
{
    directionalLights.push_back(directionalLight);
}

void RenderingScene::RemoveDirectionalLightFromScene(DirectionalLightComponent* directionalLight)
{
    auto location = std::find(directionalLights.begin(), directionalLights.end(), directionalLight);
    if (location != directionalLights.end())
    {
        directionalLights.erase(location);
    }
}

void RenderingScene::AddPointLightToScene(PointLightComponent* pointLight)
{
    pointLights.push_back(pointLight);
}

void RenderingScene::RemovePointLightFromScene(PointLightComponent* pointLight)
{
    auto location = std::find(pointLights.begin(), pointLights.end(), pointLight);
    if (location != pointLights.end())
    {
        pointLights.erase(location);
    }
}

std::vector<MeshComponent*> RenderingScene::GetSceneMeshes(const Frustum& frustum) const
{
    std::vector<MeshComponent*> meshes;

    meshOctree.FrustumCollision(frustum, meshes);
    //meshOctree.GetAllObjects(meshes);

    return meshes;
}

const std::vector<MeshComponent*>& RenderingScene::GetMeshComponents() const
{
    return meshComponents;
}

const std::vector<ReflectionProbeComponent*>& RenderingScene::GetReflectionProbeComponents() const
{
    return reflectionProbes;
}

const std::vector<DirectionalLightComponent*>& RenderingScene::GetDirectionalLights() const
{
    return directionalLights;
}

const std::vector<PointLightComponent*>& RenderingScene::GetPointLights() const
{
    return pointLights;
}

void RenderingScene::UpdateOctrees()
{
    meshOctree.Update();
}

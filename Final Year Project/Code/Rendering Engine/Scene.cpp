#include "Scene.h"

void Scene::AddToScene(GameObject* gameObject)
{
    gameObject->scene = this;
    gameObject->UpdateWorldMatrix();

    const std::vector<std::unique_ptr<BaseComponent>>& components = gameObject->GetComponents();
    for (const std::unique_ptr<BaseComponent>& component : components)
    {
        component->RegisterComponent();
    }

    for (const std::unique_ptr<BaseComponent>& component : components)
    {
        component->Update();
    }

    sceneOrganisation.push_back(gameObject);
}

void Scene::RemoveFromScene(GameObject* gameObject)
{
    auto location = std::find(sceneOrganisation.begin(), sceneOrganisation.end(), gameObject);
    if (location != sceneOrganisation.end())
    {
        gameObject->scene = nullptr;

        const std::vector<std::unique_ptr<BaseComponent>>& components = gameObject->GetComponents();
        for (const std::unique_ptr<BaseComponent>& component : components)
        {
            component->UnregisterComponent();
        }

        sceneOrganisation.erase(location);
    }
}

CameraComponent* Scene::GetActiveCamera() const
{
    return activeCamera;
}

void Scene::SetActiveCamera(CameraComponent* desiredCamera)
{
    activeCamera = desiredCamera;
}

SkyBox* Scene::GetSkyBox() const
{
    return skyBox;
}

void Scene::SetSkyBox(SkyBox* pSkyBox)
{
    skyBox = pSkyBox;
}

EnvironmentMap* Scene::GetEnvironmentMap() const
{
    return environmentMap;
}

void Scene::SetEnvironmentMap(EnvironmentMap* pEnvironmentMap)
{
    environmentMap = pEnvironmentMap;
}

void Scene::Update(float delta)
{
    for (auto sceneObject : sceneOrganisation)
    {
        sceneObject->Update();
    }

    renderingScene.UpdateOctrees();
}
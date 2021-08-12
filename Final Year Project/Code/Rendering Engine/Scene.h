#pragma once

#include "MeshComponent.h"
#include "RenderingScene.h"

#include <vector>

class CameraComponent;
struct SkyBox;
class EnvironmentMap;
class DirectionalLightComponent;

class Scene
{
public:
	void AddToScene(GameObject* gameObject);
	void RemoveFromScene(GameObject* gameObject);

	CameraComponent* GetActiveCamera() const;
	void SetActiveCamera(CameraComponent* desiredCamera);

	SkyBox* GetSkyBox() const;
	void SetSkyBox(SkyBox* skyBox);

	EnvironmentMap* GetEnvironmentMap() const;
	void SetEnvironmentMap(EnvironmentMap* environmentMap);

	void Update(float delta);

	RenderingScene renderingScene;
private:
	CameraComponent* activeCamera;

	SkyBox* skyBox;
	EnvironmentMap* environmentMap;

	std::vector<GameObject*> sceneOrganisation;
};


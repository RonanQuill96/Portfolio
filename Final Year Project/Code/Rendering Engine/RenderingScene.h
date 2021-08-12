#pragma once

#include <vector>

#include "Octree.h"

class MeshComponent;
class DirectionalLightComponent;
class PointLightComponent;
class ReflectionProbeComponent;

class RenderingScene
{
public:
	RenderingScene() : meshOctree({ {-500, -500, -500},  {500, 500, 500} }){}

	void AddMeshToScene(MeshComponent* meshComponent);
	void RemoveMeshFromScene(MeshComponent* meshComponent);
	void UpdateMesh(MeshComponent* meshComponent);

	void AddReflectionProbeToScene(ReflectionProbeComponent* reflectionProbeComponent);
	void RemoveReflectionProbeFromScene(ReflectionProbeComponent* reflectionProbeComponent);

	void AddDirectionalLightToScene(DirectionalLightComponent* directionalLight);
	void RemoveDirectionalLightFromScene(DirectionalLightComponent* directionalLight);

	void AddPointLightToScene(PointLightComponent* pointLight);
	void RemovePointLightFromScene(PointLightComponent* pointLight);

	std::vector< MeshComponent*> GetSceneMeshes(const Frustum& frustum) const;

	const std::vector< MeshComponent*>& GetMeshComponents() const;
	const std::vector< ReflectionProbeComponent* >& GetReflectionProbeComponents() const;
	const std::vector< DirectionalLightComponent* >& GetDirectionalLights() const;
	const std::vector< PointLightComponent* >& GetPointLights() const;

	void UpdateOctrees();

private:
	std::vector< MeshComponent* > meshComponents;

	Octree< MeshComponent > meshOctree;

	std::vector< DirectionalLightComponent* > directionalLights;
	std::vector< PointLightComponent* > pointLights;

	std::vector< ReflectionProbeComponent* > reflectionProbes;
};


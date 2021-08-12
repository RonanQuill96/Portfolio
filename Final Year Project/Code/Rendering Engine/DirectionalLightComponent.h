#pragma once

#include "LightComponent.h"

#include "VulkanIncludes.h"

#include "GraphicsAPI.h"
#include "UniformBuffer.h"

struct DirectionalLight
{
	glm::vec3 direction;
	float pad1;

	glm::vec3 colour;
	float pad2;
};

class DirectionalLightComponent :  public LightComponent
{
public:
	DirectionalLightComponent(GameObject& pOwner) : LightComponent(pOwner) {}

	void RegisterComponent() final;
	void UnregisterComponent() final;

	void Create(const GraphicsAPI& graphicsAPI);

	void Update() final;

	const UniformBuffer<DirectionalLight>& GetUniformBuffer() const;

private:
	UniformBuffer<DirectionalLight> perDirectionalLightBuffer;
};


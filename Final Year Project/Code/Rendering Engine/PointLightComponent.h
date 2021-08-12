#pragma once

#include "LightComponent.h"

#include "UniformBuffer.h"

#include <array>

struct PointLight
{
	glm::vec3 lightColour;
	float lightIntensity;

	glm::vec3 position;
	float range;
};

class PointLightComponent : public LightComponent
{
public:
	PointLightComponent(GameObject& pOwner) : LightComponent(pOwner) {}

	void RegisterComponent() final;
	void UnregisterComponent() final;

	void SetRange(float desiredRange);
	float GetRange() const;

	void Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool);

	void Update() final;

	const UniformBuffer<PointLight>& GetUniformBuffer() const;

private:
	UniformBuffer<PointLight> uniformBuffer;
	float range;
};


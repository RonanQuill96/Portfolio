#include "PointLightComponent.h"

#include "GameObject.h"
#include "Scene.h"

void PointLightComponent::RegisterComponent()
{
	GetOwner().GetScene()->renderingScene.AddPointLightToScene(this);
}

void PointLightComponent::UnregisterComponent()
{
	GetOwner().GetScene()->renderingScene.RemovePointLightFromScene(this);
}

void PointLightComponent::SetRange(float desiredRange)
{
	range = desiredRange;
}

float PointLightComponent::GetRange() const
{
	return range;
}

void PointLightComponent::Create(const GraphicsAPI& graphicsAPI, CommandPool& commandPool)
{
	uniformBuffer.Create(graphicsAPI);
}

void PointLightComponent::Update()
{
	PointLight pointLight;

	pointLight.lightColour = GetColour();
	pointLight.range = range;

	pointLight.position = GetOwner().GetWorldPosition();
	pointLight.lightIntensity = GetIntensity();

	uniformBuffer.Update(pointLight);
}

const UniformBuffer<PointLight>& PointLightComponent::GetUniformBuffer() const
{
	return uniformBuffer;
}

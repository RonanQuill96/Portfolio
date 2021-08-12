#include "DirectionalLightComponent.h"

#include "GameObject.h"
#include "Scene.h"

void DirectionalLightComponent::RegisterComponent()
{
	GetOwner().GetScene()->renderingScene.AddDirectionalLightToScene(this);
}

void DirectionalLightComponent::UnregisterComponent()
{
	GetOwner().GetScene()->renderingScene.RemoveDirectionalLightFromScene(this);
}

void DirectionalLightComponent::Create(const GraphicsAPI& graphicsAPI)
{
	perDirectionalLightBuffer.Create(graphicsAPI);
}

void DirectionalLightComponent::Update()
{
	DirectionalLight directionalLight;

	directionalLight.colour = GetColour();
	directionalLight.direction = GetOwner().GetForwardDirection();

	perDirectionalLightBuffer.Update(directionalLight);
}

const UniformBuffer<DirectionalLight>& DirectionalLightComponent::GetUniformBuffer() const
{
	return perDirectionalLightBuffer;
}

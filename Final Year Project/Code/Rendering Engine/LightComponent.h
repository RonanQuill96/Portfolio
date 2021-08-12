#pragma once

#include "BaseComponent.h"

#include <glm/glm.hpp>

class LightComponent : public BaseComponent
{
public:
	void SetColour(glm::vec3 desiredColour);
	glm::vec3 GetColour() const;

	void SetIntensity(float desiredIntensity);
	float GetIntensity() const;

protected:
	LightComponent(GameObject& pOwner) : BaseComponent(pOwner) {}

private:
	glm::vec3 colour;
	float intensity;
};


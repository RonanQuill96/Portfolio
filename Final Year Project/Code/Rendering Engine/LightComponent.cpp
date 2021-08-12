#include "LightComponent.h"

void LightComponent::SetColour(glm::vec3 desiredColour)
{
    colour = desiredColour;
}

glm::vec3 LightComponent::GetColour() const
{
    return colour;
}

void LightComponent::SetIntensity(float desiredIntensity)
{
    intensity = desiredIntensity;
}

float LightComponent::GetIntensity() const
{
    return intensity;
}

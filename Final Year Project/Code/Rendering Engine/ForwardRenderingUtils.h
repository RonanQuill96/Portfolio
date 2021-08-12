#pragma once

#include "VulkanIncludes.h"

struct PerFrameInfo
{
	glm::vec3 cameraPosition;
	float pad;
	glm::vec3 lightDirection;
	float pad1;
	glm::vec3 lightColour;
	float lightIntensity;
};

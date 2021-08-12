#pragma once

#include <glm/glm.hpp>

struct PointLightIn
{
	glm::vec3 lightColour;
	float lightIntensity;

	glm::vec3 position;
	float range;
};

#define MAX_LIGHT_PER_PASS 10000
struct PointLightInfo
{
	int count;
	int pad1;
	int pad2;
	int pad3;
	PointLightIn pointLights[MAX_LIGHT_PER_PASS];
};

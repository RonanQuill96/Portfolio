#pragma once

#include <glm/glm.hpp>

#include <array>

struct Frustum
{
	std::array<glm::vec4, 6> plane;
};
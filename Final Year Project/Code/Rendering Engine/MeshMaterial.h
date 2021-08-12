#pragma once

#include "Material.h"

#include <memory>

struct MeshMaterial
{
	std::shared_ptr<Material> material = nullptr;

	uint32_t startIndex = 0;
	uint32_t indexCount = 0;
};
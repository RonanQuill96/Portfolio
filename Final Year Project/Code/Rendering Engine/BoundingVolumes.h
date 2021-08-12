#pragma once

#include "AABB.h"
#include "Frustum.h"

#include <glm/glm.hpp>

class BoundingVolume
{
public:
	static bool AABBContains(const AABB& container, const AABB& subject); //Bounding volumes must be in world space
	static bool AABBOverlap(const AABB& firstAABB, const AABB& subject); //Bounding volumes must be in world space
	static bool AABBFustrumCollision(const AABB& aabb, const Frustum& frustum); //Bounding volumes must be in world space

	static void UpdateAABB(const AABB& originalAABB, const glm::mat4& worldMatrix, AABB& newAABB);
};
#include "BoundingVolumes.h"

bool BoundingVolume::AABBContains(const AABB& container, const AABB& subject)
{
	return (subject.minVertex.x > container.minVertex.x && subject.maxVertex.x < container.maxVertex.x&&
			subject.minVertex.z > container.minVertex.z && subject.maxVertex.z < container.maxVertex.z&&
			subject.minVertex.y > container.minVertex.y && subject.maxVertex.y < container.maxVertex.y);
}

bool BoundingVolume::AABBOverlap(const AABB& firstAABB, const AABB& subject)
{
	if (firstAABB.maxVertex.x < subject.minVertex.x || firstAABB.minVertex.x > subject.maxVertex.x)
		return false;
	if (firstAABB.maxVertex.z < subject.minVertex.z || firstAABB.minVertex.z > subject.maxVertex.z)
		return false;
	if (firstAABB.maxVertex.y < subject.minVertex.y || firstAABB.minVertex.y > subject.maxVertex.y)
		return false;

	return true;
}

bool BoundingVolume::AABBFustrumCollision(const AABB& aabb, const Frustum& frustum)
{
	for (const glm::vec4& plane : frustum.plane)
	{
		const glm::vec3 planeNormal = { plane.x, plane.y, plane.z };

		glm::vec3 axisVert;

		// x-axis
		if (planeNormal.x < 0.0f)	// Which AABB vertex is furthest down (plane normals direction) the x axis
		{
			axisVert.x = aabb.minVertex.x;
		}
		else
		{
			axisVert.x = aabb.maxVertex.x;
		}

		// y-axis
		if (planeNormal.y < 0.0f)	// Which AABB vertex is furthest down (plane normals direction) the y axis
		{
			axisVert.y = aabb.minVertex.y;
		}
		else
		{
			axisVert.y = aabb.maxVertex.y;
		}

		// z-axis
		if (planeNormal.z < 0.0f)	// Which AABB vertex is furthest down (plane normals direction) the z axis
		{
			axisVert.z = aabb.minVertex.z;
		}
		else
		{
			axisVert.z = aabb.maxVertex.z;
		}

		// Now we get the signed distance from the AABB vertex that's furthest down the frustum planes normal,
		// and if the signed distance is negative, then the entire bounding box is behind the frustum plane, which means
		// that it should be culled
		if (glm::dot(planeNormal, axisVert) + plane.w < 0.0f)
		{
			return false;
		}
	}

	return true;
}

void BoundingVolume::UpdateAABB(const AABB& originalAABB, const glm::mat4& worldMatrix, AABB& newAABB)
{
	newAABB.minVertex = newAABB.maxVertex = glm::vec3(worldMatrix[3]);

	for (uint16_t column = 0; column < 3; ++column)
	{
		// Form extent by summing smaller and larger terms respectively
		for (uint16_t row = 0; row < 3; ++row)
		{
			const float e = worldMatrix[row][column] * originalAABB.minVertex[row];
			const float f = worldMatrix[row][column] * originalAABB.maxVertex[row];

			if (e < f)
			{
				newAABB.minVertex[column] += e;
				newAABB.maxVertex[column] += f;
			}
			else
			{
				newAABB.minVertex[column] += f;
				newAABB.maxVertex[column] += e;
			}
		}
	}
}

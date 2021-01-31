#pragma once

#include "Ray.h"
#include "Vector3.h"

#include <algorithm>
#include <utility>

class Hittable;

class AABB
{
public:
	AABB() = default;
	AABB(const Vector3& a, const Vector3& b);

	Vector3 Min() const;

	Vector3 Max() const;

	bool RayIntersection(const Ray& ray, float tmin, float tmax) const;

private:
	Vector3 min;
	Vector3 max;

public:
	static AABB SurroundingBox(AABB box0, AABB box1);

	//Used for sort fuinction in BVHNode
	static int AABBAxisXComparison(const Hittable* a, const Hittable* b);
	static int AABBAxisYComparison(const Hittable* a, const Hittable* b);
	static int AABBAxisZComparison(const Hittable* a, const Hittable* b);

};
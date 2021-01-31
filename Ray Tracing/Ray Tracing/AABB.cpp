#include "AABB.h"

#include "Hittable.h"

AABB::AABB(const Vector3& a, const Vector3& b)
{
	min = a;
	max = b;
}

Vector3 AABB::Min() const
{
	return min;
}

Vector3 AABB::Max() const
{
	return max;
}

bool AABB::RayIntersection(const Ray& ray, float tmin, float tmax) const
{
	for (int i = 0; i < 3; i++)
	{
		float invD = 1.0f / ray.Direction().v[i];
		float t0 = (min.v[i] - ray.Origin().v[i]) * invD;
		float t1 = (max.v[i] - ray.Origin().v[i]) * invD;
		if (invD < 0.0f)
			std::swap(t0, t1);

		tmin = t0 > tmin ? t0 : tmin;
		tmax = t1 < tmax ? t1 : tmax;

		if (tmax <= tmin)
			return false;
	}
	return true;
}

AABB AABB::SurroundingBox(AABB box0, AABB box1)
{
	Vector3 min(std::min(box0.min.x, box1.min.x),
		std::min(box0.min.y, box1.min.y),
		std::min(box0.min.z, box1.min.z));

	Vector3 max(std::max(box0.max.x, box1.max.x),
		std::max(box0.max.y, box1.max.y),
		std::max(box0.max.z, box1.max.z));

	return AABB(min, max);
}

int AABB::AABBAxisXComparison(const Hittable* a, const Hittable* b)
{
	constexpr int axis = 0;
	AABB boxLeft;
	AABB boxRight;
	a->BoundingBox(0, 0, boxLeft);
	b->BoundingBox(0, 0, boxRight);

	if (boxLeft.Min().v[axis] - boxRight.Min().v[axis] < 0.0f)
		return false;
	else
		return true;
}

int AABB::AABBAxisYComparison(const Hittable* a, const Hittable* b)
{
	constexpr int axis = 1;
	AABB boxLeft;
	AABB boxRight;
	a->BoundingBox(0, 0, boxLeft);
	b->BoundingBox(0, 0, boxRight);

	if (boxLeft.Min().v[axis] - boxRight.Min().v[axis] < 0.0f)
		return false;
	else
		return true;
}

int AABB::AABBAxisZComparison(const Hittable* a, const Hittable* b)
{
	constexpr int axis = 2;
	AABB boxLeft;
	AABB boxRight;
	a->BoundingBox(0, 0, boxLeft);
	b->BoundingBox(0, 0, boxRight);

	if (boxLeft.Min().v[axis] - boxRight.Min().v[axis] < 0.0f)
		return false;
	else
		return true;
}

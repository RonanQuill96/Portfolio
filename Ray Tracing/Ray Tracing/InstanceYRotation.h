#pragma once

#include "Hittable.h"
#include "Vector3.h"
#include "Ray.h"
#include "Util.h"

class InstanceYRotation : public Hittable
{
public:
	InstanceYRotation(Hittable* pShape, float angle);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Hittable* shape;
	float sinTheta;
	float cosTheta;
	bool hasbox;
	AABB bbox;
};
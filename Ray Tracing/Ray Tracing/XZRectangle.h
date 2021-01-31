#pragma once

#include "Hittable.h"

class XZRectangle : public Hittable
{
public:
	XZRectangle() = default;
	XZRectangle(float _x0, float _x1, float _z0, float _z1, float _k, Material* mat);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Material* mp;
	float x0, x1, z0, z1, k;
};

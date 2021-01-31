#pragma once

#include "Hittable.h"

class YZRectangle : public Hittable
{
public:
	YZRectangle() = default;
	YZRectangle(float _y0, float _y1, float _z0, float _z1, float _k, Material* mat);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Material* mp;
	float y0, y1, z0, z1, k;
};

#pragma once

#include "Hittable.h"
#include "Material.h"

class XYRectangle : public Hittable
{
public:
	XYRectangle() = default;
	XYRectangle(float _x0, float _x1, float _y0, float _y1, float _k, Material* mat);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Material* mp;
	float x0, x1, y0, y1, k;
};
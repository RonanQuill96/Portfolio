#pragma once

#include "Hittable.h"

class HittableList : public Hittable
{
public:
	HittableList() = default;
	HittableList(Hittable** l, int n);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Hittable** list;
	size_t size;
};

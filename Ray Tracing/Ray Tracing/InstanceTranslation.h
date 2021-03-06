#pragma once

#include "Hittable.h"
#include "Vector3.h"
#include "Ray.h"

class InstanceTranslation : public Hittable
{
public:
	InstanceTranslation(Hittable* pShape, const Vector3& pTranlation);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
	Hittable* shape;
	Vector3 translation;
};
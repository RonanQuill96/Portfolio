#pragma once

#include "Hittable.h"

class MovingSphere : public Hittable
{
public:
	MovingSphere() = default;
	MovingSphere(Vector3 cen0, Vector3 cen1, float t0, float t1, float r, Material* m);

	bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
	bool BoundingBox(float t0, float t1, AABB& box) const override;

	Vector3 Center(float time) const;

private:
	float time0, time1;
	Vector3 center0, center1;
	float radius;
	Material* material;
};
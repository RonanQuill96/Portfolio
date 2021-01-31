#pragma once

#include "AABB.h"
#include "Ray.h"

class Material;

struct HitRecord
{
	float t;
	float u = 0.0;
	float v = 0.0;
	Vector3 p;
	Material* materialPtr;
	Vector3 normal;
	bool frontFace;

	inline void SetFaceNormal(const Ray& r, const Vector3& outward_normal) 
	{
		frontFace = DotProduct(r.Direction(), outward_normal) < 0;
		normal = frontFace ? outward_normal : -outward_normal;
	}
};

class Hittable
{
public:
	virtual ~Hittable() = default;

	virtual bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const = 0;
	virtual bool BoundingBox(float t0, float t1, AABB& box) const = 0;
};
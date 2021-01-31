#include "XZRectangle.h"

XZRectangle::XZRectangle(float _x0, float _x1, float _z0, float _z1, float _k, Material* mat) 
	: x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat) 
{}

bool XZRectangle::Hit(const Ray & r, float tMin, float tMax, HitRecord & hitRecord) const
{
	auto t = (k - r.Origin().y) / r.Direction().y;

	if (t < tMin || t > tMax)
	{
		return false;
	}

	auto x = r.Origin().x + t * r.Direction().x;
	auto z = r.Origin().z + t * r.Direction().z;

	if (x < x0 || x > x1 || z < z0 || z > z1)
	{
		return false;
	}


	hitRecord.u = (x - x0) / (x1 - x0);
	hitRecord.v = (z - z0) / (z1 - z0);
	hitRecord.t = t;
	hitRecord.materialPtr = mp;
	Vector3 outwardNormal = Vector3(0.0f, 1.0f, 0.0f);
	hitRecord.SetFaceNormal(r, outwardNormal);
	hitRecord.p = r.PointAtTime(t);

	return true;
}

bool XZRectangle::BoundingBox(float t0, float t1, AABB& box) const
{
	box = AABB(Vector3(x0, k - 0.00001f, z0), Vector3(x1, k + 0.00001f, z1));
	return true;
}

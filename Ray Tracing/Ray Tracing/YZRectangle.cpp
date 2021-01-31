#include "YZRectangle.h"

YZRectangle::YZRectangle(float _y0, float _y1, float _z0, float _z1, float _k, Material* mat) 
	: y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat) {}

bool YZRectangle::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	float t = (k - r.Origin().x) / r.Direction().x;

	if (t < tMin || t > tMax)
	{
		return false;
	}

	float y = r.Origin().y + t * r.Direction().y;
	float z = r.Origin().z + t * r.Direction().z;

	if (y < y0 || y > y1 || z < z0 || z > z1)
	{
		return false;
	}

	hitRecord.u = (y - y0) / (y1 - y0);
	hitRecord.v = (z - z0) / (z1 - z0);
	hitRecord.t = t;
	hitRecord.materialPtr = mp;
	Vector3 outwardNormal = Vector3(1.0f, 0.0f, 0.0f);
	hitRecord.SetFaceNormal(r, outwardNormal);
	hitRecord.p = r.PointAtTime(t);

	return true;
}

bool YZRectangle::BoundingBox(float t0, float t1, AABB& box) const
{
	box = AABB(Vector3(k - 0.00001f, y0, z0), Vector3(k + 0.00001f, y1, z1));
	return true;
}

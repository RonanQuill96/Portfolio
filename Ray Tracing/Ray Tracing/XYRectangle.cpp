#include "XYRectangle.h"

XYRectangle::XYRectangle(float _x0, float _x1, float _y0, float _y1, float _k, Material* mat) 
	: x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {}

bool XYRectangle::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	float t = (k - r.Origin().z) / r.Direction().z;

	if (t < tMin || t > tMax)
	{
		return false;
	}

	float x = r.Origin().x + t * r.Direction().x;
	float y = r.Origin().y + t * r.Direction().y;

	if (x < x0 || x > x1 || y < y0 || y > y1)
	{
		return false;
	}

	hitRecord.u = (x - x0) / (x1 - x0);
	hitRecord.v = (y - y0) / (y1 - y0);
	hitRecord.t = t;
	hitRecord.materialPtr = mp;
	Vector3 outwardNormal = Vector3(0.0f, 0.0f, 1.0f);
	hitRecord.SetFaceNormal(r, outwardNormal);
	hitRecord.p = r.PointAtTime(t);

	return true;
}

bool XYRectangle::BoundingBox(float t0, float t1, AABB& box) const
{
	box = AABB(Vector3(x0, y0, k - 0.00001f), Vector3(x1, y1, k + 0.00001f));
	return true;
}
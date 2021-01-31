#include "MovingSphere.h"

MovingSphere::MovingSphere(Vector3 cen0, Vector3 cen1, float t0, float t1, float r, Material* m) :
	center0(cen0), center1(cen1), time0(t0), time1(t1), radius(r), material(m) {}

bool MovingSphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	const Vector3 oc = r.Origin() - Center(r.GetTime());

	const float a = DotProduct(r.Direction(), r.Direction());
	const float b = DotProduct(oc, r.Direction());
	const float c = DotProduct(oc, oc) - radius * radius;

	const float discriminant = b * b - a * c;

	if (discriminant > 0.0f)
	{
		float temp = (-b - std::sqrt(discriminant)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitRecord.t = temp;
			hitRecord.p = r.PointAtTime(hitRecord.t);
			Vector3 outwardNormal = (hitRecord.p - Center(r.GetTime())) / radius;
			hitRecord.SetFaceNormal(r, outwardNormal);
			hitRecord.materialPtr = material;
			return true;
		}

		temp = (-b + std::sqrt(discriminant)) / a;
		if (temp < tMax && temp > tMin)
		{
			hitRecord.t = temp;
			hitRecord.p = r.PointAtTime(hitRecord.t);
			Vector3 outwardNormal = (hitRecord.p - Center(r.GetTime())) / radius;
			hitRecord.SetFaceNormal(r, outwardNormal);
			hitRecord.materialPtr = material;
			return true;
		}
	}

	return false;
}

Vector3 MovingSphere::Center(float time) const
{
	return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
}

bool MovingSphere::BoundingBox(float t0, float t1, AABB& box) const
{
	AABB box0 = AABB(Center(t0) - Vector3(radius, radius, radius), Center(t0) + Vector3(radius, radius, radius));
	AABB box1 = AABB(Center(t1) - Vector3(radius, radius, radius), Center(t1) + Vector3(radius, radius, radius));

	box = AABB::SurroundingBox(box0, box1);

	return true;
}

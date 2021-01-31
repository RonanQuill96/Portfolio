#include "InstanceYRotation.h"

InstanceYRotation::InstanceYRotation(Hittable* pShape, float angle) : shape(pShape)
{
	float radians = Util::DegreesToRadians(angle);
	sinTheta = std::sin(radians);
	cosTheta = std::cos(radians);
	hasbox = shape->BoundingBox(0, 1, bbox);

	Vector3 min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vector3 max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 2; k++) {
				auto x = i * bbox.Max().x + (1 - i) * bbox.Min().x;
				auto y = j * bbox.Max().y + (1 - j) * bbox.Min().y;
				auto z = k * bbox.Max().z + (1 - k) * bbox.Min().z;

				float newx = cosTheta * x + sinTheta * z;
				float newz = -sinTheta * x + cosTheta * z;

				Vector3 tester(newx, y, newz);

				for (int c = 0; c < 3; c++) {
					min.v[c] = std::min(min.v[c], tester.v[c]);
					max.v[c] = std::max(max.v[c], tester.v[c]);
				}
			}
		}
	}

	bbox = AABB(min, max);
}

bool InstanceYRotation::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	Vector3 origin = r.Origin();
	Vector3 direction = r.Direction();

	origin[0] = cosTheta * r.Origin()[0] - sinTheta * r.Origin()[2];
	origin[2] = sinTheta * r.Origin()[0] + cosTheta * r.Origin()[2];

	direction[0] = cosTheta * r.Direction()[0] - sinTheta * r.Direction()[2];
	direction[2] = sinTheta * r.Direction()[0] + cosTheta * r.Direction()[2];

	Ray rotated_r(origin, direction, r.GetTime());

	if (!shape->Hit(rotated_r, tMin, tMax, hitRecord))
		return false;

	Vector3 p = hitRecord.p;
	Vector3 normal = hitRecord.normal;

	p[0] = cosTheta * hitRecord.p[0] + sinTheta * hitRecord.p[2];
	p[2] = -sinTheta * hitRecord.p[0] + cosTheta * hitRecord.p[2];

	normal[0] = cosTheta * hitRecord.normal[0] + sinTheta * hitRecord.normal[2];
	normal[2] = -sinTheta * hitRecord.normal[0] + cosTheta * hitRecord.normal[2];

	hitRecord.p = p;
	hitRecord.SetFaceNormal(rotated_r, normal);

	return true;
}

bool InstanceYRotation::BoundingBox(float t0, float t1, AABB& box) const
{
	box = bbox;
	return hasbox;
}

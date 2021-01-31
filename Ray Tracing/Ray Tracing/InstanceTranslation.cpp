#include "InstanceTranslation.h"

InstanceTranslation::InstanceTranslation(Hittable* pShape, const Vector3& pTranlation) 
	: shape(pShape), translation(pTranlation) {}

bool InstanceTranslation::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
	Ray moved_r(r.Origin() - translation, r.Direction(), r.GetTime());
	if (!shape->Hit(moved_r, tMin, tMax, hitRecord))
		return false;

	hitRecord.p += translation;
	hitRecord.SetFaceNormal(moved_r, hitRecord.normal);

	return true;
}

bool InstanceTranslation::BoundingBox(float t0, float t1, AABB& box) const
{
	if (!shape->BoundingBox(t0, t1, box))
		return false;

	box = AABB(box.Min() + translation, box.Max() + translation);

	return true;
}
#include "DiffuseLight.h"

bool DiffuseLight::Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const
{
	return false;
}

Vector3 DiffuseLight::Emitted(float u, float v, const Vector3& p) const
{
	return emit->Value(u, v, p);
}

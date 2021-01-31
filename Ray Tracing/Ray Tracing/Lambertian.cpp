#include "Lambertian.h"

bool Lambertian::Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const
{
	Vector3 target = hitRecord.p + hitRecord.normal + Util::RandomInUnitSphere();
	scattered = Ray(hitRecord.p, target - hitRecord.p, r_in.GetTime());
	attenuation = albedo->Value(hitRecord.u, hitRecord.v, hitRecord.p);
	return true;
}

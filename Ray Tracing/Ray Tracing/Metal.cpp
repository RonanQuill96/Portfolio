#include "Metal.h"

Metal::Metal(const Vector3& a, float f) : albedo(a), fuzz(std::clamp(f, 0.0f, 1.0f))
{
}

bool Metal::Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const
{
	Vector3 reflected = Util::Reflect(GetNormalized(r_in.Direction()), hitRecord.normal);
	scattered = Ray(hitRecord.p, reflected, 0.0f);
	attenuation = albedo;
	return DotProduct(scattered.Direction(), hitRecord.normal) > 0.0f;
}
#include "Dialectric.h"

Dialectric::Dialectric(float ri) : refractionIndex(ri) 
{
}

bool Dialectric::Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const

{
	Vector3 outwardsNormal;
	Vector3 reflected = Util::Reflect(r_in.Direction(), hitRecord.normal);
	float ni_over_nt;
	attenuation = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 refracted;

	float reflectProb;
	float cosine;

	if (DotProduct(r_in.Direction(), hitRecord.normal) > 0.0f)
	{
		outwardsNormal = -hitRecord.normal;
		ni_over_nt = refractionIndex;
		cosine = refractionIndex * DotProduct(r_in.Direction(), hitRecord.normal) / r_in.Direction().Length();
	}
	else
	{
		outwardsNormal = hitRecord.normal;
		ni_over_nt = 1.0f / refractionIndex;
		cosine = -DotProduct(r_in.Direction(), hitRecord.normal) / r_in.Direction().Length();
	}

	if (Util::Refract(r_in.Direction(), outwardsNormal, ni_over_nt, refracted))
	{
		reflectProb = Util::Schlick(cosine, refractionIndex);
	}
	else
	{
		scattered = Ray(hitRecord.p, reflected, 0.0f);
		reflectProb = 1.0f;
	}

	if (Util::RandomFloat() < reflectProb)
	{
		scattered = Ray(hitRecord.p, reflected, 0.0f);
	}
	else
	{
		scattered = Ray(hitRecord.p, refracted, 0.0f);
	}

	return true;
}

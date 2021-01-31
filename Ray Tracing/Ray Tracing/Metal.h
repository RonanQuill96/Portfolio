#pragma once

#include "Material.h"
#include "Util.h"

class Metal : public Material
{
public:
	Metal(const Vector3& a, float f);

	bool Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const override;

private:
	Vector3 albedo;
	float fuzz;
};
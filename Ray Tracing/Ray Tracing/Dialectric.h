#pragma once

#include "Material.h"
#include "Util.h"

class Dialectric : public Material
{
public:
	Dialectric(float ri);

	bool Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const override;

private:
	float refractionIndex;
};
#pragma once

#include "Material.h"
#include "Util.h"

#include "Texture.h"

class Lambertian : public Material
{
public:
	Lambertian(Texture* a) : albedo(a) {}

	bool Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const override;

private:
	Texture* albedo;
};
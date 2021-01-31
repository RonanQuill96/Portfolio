#pragma once

#include "Material.h"
#include "Texture.h"

class DiffuseLight : public Material
{
public:
	DiffuseLight(Texture* a) : emit(a) {}

	bool Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const override;

	Vector3 Emitted(float u, float v, const Vector3& p) const override;

private:
	Texture* emit;
};
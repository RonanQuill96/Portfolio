#pragma once

#include "Hittable.h"
#include "Ray.h"

class Material
{
public:
	virtual ~Material() = default;

	virtual bool Scatter(const Ray& r_in, const HitRecord& hitRecord, Vector3& attenuation, Ray& scattered) const = 0;
	virtual Vector3 Emitted(float u, float v, const Vector3& p) const;
};
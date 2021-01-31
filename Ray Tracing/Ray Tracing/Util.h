#pragma once

#include "Vector3.h"

namespace Util
{
	constexpr float R_PI = 3.14159265359f;

	float DegreesToRadians(float degrees);

	float RandomFloat();
	Vector3 RandomInUnitSphere();
	Vector3 RandomInUnitDisk();

	Vector3 Reflect(const Vector3& v1, const Vector3& v2);

	bool Refract(const Vector3& v, const Vector3& n, float ni_over_nt, Vector3& refracted);

	float Schlick(float cosine, float refractionIndex);

	void GetSphereUV(const Vector3& p, float& u, float& v);
}

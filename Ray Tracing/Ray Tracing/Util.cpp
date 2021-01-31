#include "Util.h"

#include <random>

float Util::DegreesToRadians(float degrees)
{
	return degrees * R_PI / 180.0f;
}

float Util::RandomFloat()
{
	static std::random_device rd;
	static std::mt19937 mt(rd());
	static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	return dist(mt);
}

Vector3 Util::RandomInUnitSphere()
{
	Vector3 p;

	do
	{
		p = 2.0 * Vector3(RandomFloat(), RandomFloat(), RandomFloat()) - Vector3(1.0f, 1.0f, 1.0f);
	} while (p.SquaredLength() >= 1.0f);

	return p;
}

Vector3 Util::RandomInUnitDisk()
{
	Vector3 p;

	do
	{
		p = 2.0 * Vector3(RandomFloat(), RandomFloat(), 0.0f) - Vector3(1.0f, 1.0f, 0.0f);
	} while (DotProduct(p, p) >= 1.0f);

	return p;
}

Vector3 Util::Reflect(const Vector3& v1, const Vector3& v2)
{
	return v1 - 2 * DotProduct(v1, v2) * v2;
}

bool Util::Refract(const Vector3& v, const Vector3& n, float ni_over_nt, Vector3& refracted)
{
	Vector3 uv = GetNormalized(v);
	float dt = DotProduct(uv, n);
	float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1 - dt * dt);
	if (discriminant > 0)
	{
		refracted = ni_over_nt * (uv - n * dt) - n * std::sqrt(discriminant);
		return true;
	}
	else
	{
		return false;
	}
}

float Util::Schlick(float cosine, float refractionIndex)
{
	float r0 = (1.0f - refractionIndex) / (1.0f + refractionIndex);
	r0 = r0 * r0;
	return r0 + (1.0f - r0) * std::pow((1.0f - cosine), 5.0f);
}

void Util::GetSphereUV(const Vector3& p, float& u, float& v)
{
	float phi = std::atan2(p.z, p.x);
	float theta = std::asin(p.y);

	u = 1.0f - (phi * R_PI) / (2 * R_PI);
	v = (theta + R_PI / 2.0f) / R_PI;
}
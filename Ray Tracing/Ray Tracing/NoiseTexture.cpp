#include "NoiseTexture.h"

NoiseTexture::NoiseTexture(float sc)
	: scale(sc) {}

Vector3 NoiseTexture::Value(float u, float v, const Vector3& p) const
{
	//return Vector3(1.0f, 1.0f, 1.0f) * noise.Generate(scale * p);
	return Vector3(1.0f, 1.0f, 1.0f) * 0.5 * (1.0f + std::sin(scale * p.z + 10.0f * noise.Turbulence(p)));
}

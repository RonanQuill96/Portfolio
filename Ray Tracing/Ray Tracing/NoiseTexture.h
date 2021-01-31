#pragma once

#include "Texture.h"

#include "PerlinNoise.h"

class NoiseTexture : public Texture
{
public:
	NoiseTexture() = default;
	NoiseTexture(float sc);

	Vector3 Value(float u, float v, const Vector3& p) const override;

private:
	PerlinNoise noise;
	float scale = 1.0f;
};
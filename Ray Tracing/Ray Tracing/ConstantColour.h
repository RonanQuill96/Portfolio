#pragma once

#include "Texture.h"

class ConstantColour : public Texture
{
public:
	ConstantColour() = default;
	ConstantColour(Vector3 c);

	Vector3 Value(float u, float v, const Vector3& p) const override;

private:
	Vector3 colour;
};
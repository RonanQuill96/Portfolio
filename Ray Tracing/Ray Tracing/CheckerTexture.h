#pragma once

#include "Texture.h"

#include <algorithm>

class CheckerTexture : public Texture
{
public:
	CheckerTexture() = default;
	CheckerTexture(Texture* t0, Texture* t1);

	Vector3 Value(float u, float v, const Vector3& p) const override;

private:
	Texture* odd;
	Texture* even;
};
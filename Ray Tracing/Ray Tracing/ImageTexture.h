#pragma once

#include "Texture.h"

#include "Vector3.h"
#include "Util.h"

#include <algorithm>

class ImageTexture : public Texture
{
public:
	ImageTexture() = default;
	ImageTexture(unsigned char* pixels, int A, int B);

	Vector3 Value(float u, float v, const Vector3& p) const override;

private:
	unsigned char* data;
	int nx;
	int ny;
};

#include "ImageTexture.h"

ImageTexture::ImageTexture(unsigned char* pixels, int A, int B) 
	: data(pixels), nx(A), ny(B) {}

Vector3 ImageTexture::Value(float u, float v, const Vector3& p) const
{
	int i = (u)*nx;
	int j = (1 - v) * ny - 0.0001f;

	i = std::clamp(i, 0, nx - 1);
	j = std::clamp(j, 0, ny - 1);

	float r = int(data[3 * i + 3 * nx * j]) / 255.0f;
	float g = int(data[3 * i + 3 * nx * j + 1]) / 255.0f;
	float b = int(data[3 * i + 3 * nx * j + 2]) / 255.0f;

	return Vector3(r, g, b);
}

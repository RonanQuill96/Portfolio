#include "CheckerTexture.h"

CheckerTexture::CheckerTexture(Texture* t0, Texture* t1) : even(t0), odd(t1) {}

Vector3 CheckerTexture::Value(float u, float v, const Vector3& p) const
{
	float sine = std::sin(10 * p.x) * std::sin(10 * p.y) * std::sin(10 * p.z);

	if (sine < 0.0f)
		return odd->Value(u, v, p);
	else
		return even->Value(u, v, p);
}

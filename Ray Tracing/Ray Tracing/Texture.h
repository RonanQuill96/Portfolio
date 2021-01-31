#pragma once

#include "Vector3.h"

class Texture
{
public:
	virtual ~Texture() = default;

	virtual Vector3 Value(float u, float v, const Vector3& p) const = 0;
};
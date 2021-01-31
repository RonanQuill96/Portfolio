#pragma once

#include "Vector3.h"

class Ray
{
public:
	Ray() = default;
	Ray(const Vector3& a, const Vector3& b, float ti);

	Vector3 Origin() const;
	Vector3 Direction() const;
	float GetTime() const;
	Vector3 PointAtTime(float t) const;

private:
	Vector3 origin;
	Vector3 direction;
	float _time = 0.0f;
};
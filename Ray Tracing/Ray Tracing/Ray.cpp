#include "Ray.h"

Ray::Ray(const Vector3& a, const Vector3& b, float ti)
{
	origin = a;
	direction = b;
	_time = ti;
}

Vector3 Ray::Origin() const
{
	return origin;
}

Vector3 Ray::Direction() const
{
	return direction;
}

float Ray::GetTime() const
{
	return _time;
}

Vector3 Ray::PointAtTime(float t) const
{
	return origin + t * direction;
}


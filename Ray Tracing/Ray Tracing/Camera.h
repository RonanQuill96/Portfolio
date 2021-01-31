#pragma once

#include "Ray.h"
#include "Util.h"

class Camera
{
public:
	Camera(Vector3 lookfrom, Vector3 lookat, Vector3 vup, float vfov, float aspect, float aperture, float focusDist, float t0, float t1);

	Ray GetRay(float s, float t) const;

private:
	Vector3 lowerLeftCorner;
	Vector3 horizontal;
	Vector3 vertical;
	Vector3 origin;
	Vector3 u, v, w;
	float lensRadius;

	float time0;
	float time1;
};
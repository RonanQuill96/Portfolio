#include "Camera.h"

Camera::Camera(Vector3 lookfrom, Vector3 lookat, Vector3 vup, float vfov, float aspect, float aperture, float focusDist, float t0, float t1)
{
	time0 = t0;
	time1 = t1;

	float theta = Util::DegreesToRadians(vfov);
	float h = std::tan(theta / 2.0f);
	float viewport_height = 2.0f * h;
	float viewport_width = aspect * viewport_height;

	w = GetNormalized(lookfrom - lookat);
	u = GetNormalized(CrossProduct(vup, w));
	v = CrossProduct(w, u);

	origin = lookfrom;
	horizontal = focusDist * viewport_width * u;
	vertical = focusDist * viewport_height * v;
	lowerLeftCorner = origin - horizontal / 2.0f - vertical / 2.0f - focusDist * w;

	lensRadius = aperture / 2.0f;
}

Ray Camera::GetRay(float s, float t) const
{
	Vector3 rd = lensRadius * Util::RandomInUnitDisk();
	Vector3 offset = u * rd.x + v * rd.y;
	float time = time0 + Util::RandomFloat() * (time1 - time0);
	return Ray(origin + offset, lowerLeftCorner + s * horizontal + t * vertical - origin - offset, time);
}

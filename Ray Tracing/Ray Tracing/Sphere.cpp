#include "Sphere.h"

Sphere::Sphere(Vector3 cen, float r, Material* m)
    : center(cen), radius(r), material(m) {}

bool Sphere::Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const
{
    SIMD::Vector rO = { r.Origin().x, r.Origin().y, r.Origin().z, 0.0f };
    SIMD::Vector rD = { r.Direction().x, r.Direction().y, r.Direction().z, 0.0f };
    SIMD::Vector sC = { center.x, center.y, center.z, 0.0f };

    const SIMD::Vector oc = rO - sC;

    const float a = SIMD::Vector::DotProduct(rD, rD);
    const float b = SIMD::Vector::DotProduct(oc, rD);
    const float c = SIMD::Vector::DotProduct(oc, oc) - radius * radius;

    const float discriminant = b * b - a * c;

    if (discriminant > 0.0f)
    {
        float temp = (-b - std::sqrt(discriminant)) / a;
        if (temp < tMax && temp > tMin)
        {
            hitRecord.t = temp;
            hitRecord.p = r.PointAtTime(hitRecord.t);
            Vector3 outwardNormal = (hitRecord.p - center) / radius;
            hitRecord.SetFaceNormal(r, outwardNormal);
            hitRecord.materialPtr = material;
            return true;
        }

        temp = (-b + std::sqrt(discriminant)) / a;
        if (temp < tMax && temp > tMin)
        {
            hitRecord.t = temp;
            hitRecord.p = r.PointAtTime(hitRecord.t);
            Vector3 outwardNormal = (hitRecord.p - center) / radius;
            hitRecord.SetFaceNormal(r, outwardNormal);
            hitRecord.materialPtr = material;
            return true;
        }
    }

    return false;
}

bool Sphere::BoundingBox(float t0, float t1, AABB& box) const
{
    box = AABB(center - Vector3(radius, radius, radius), center + Vector3(radius, radius, radius));
    return true;
}
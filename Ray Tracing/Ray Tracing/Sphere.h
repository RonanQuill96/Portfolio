#pragma once

#include "Hittable.h"

#include <Vector.h>

class Sphere : public Hittable
{
public:
    Sphere() = default;
    Sphere(Vector3 cen, float r, Material* m);

    bool Hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) const override;
    bool BoundingBox(float t0, float t1, AABB& box) const override;

private:
    Vector3 center;
    float radius;
    Material* material;
};
